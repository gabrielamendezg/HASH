#include "hash.h"
#include "lista.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define TAM_INICIAL 50
#define FACTOR_CAP 5
#define FACTOR_REDIM 2
#define FACTOR_MIN_CAP 4

/**********************************************
 * 				HASH_CAMPO					  *
 * ********************************************/
 
typedef struct hash_campo{
	char* clave;
	void* dato;
}hash_campo_t;
 
//Crea un campo con la clave y dato asignados. 
hash_campo_t* hash_campo_crear(const char* clave,void* dato){
	hash_campo_t* campo = malloc(sizeof(hash_campo_t));
	if(campo==NULL)return NULL;
	
	campo->dato = dato;
	size_t largo_str = strlen(clave)+1;
	char* str = malloc(sizeof(char)*largo_str);
	if(str==NULL){
		free(campo);
		return NULL;
	}
	memcpy(str,clave,largo_str);
	campo->clave = str;
	return campo;
}
 
void hash_campo_destruir(hash_campo_t* campo, void destruir_dato(void*)){
	if(destruir_dato!=NULL)destruir_dato(campo->dato);
	free(campo->clave);
	free(campo);
}
 
/**********************************************
 * 				    HASH					  *
 * ********************************************/
struct hash{
	lista_t** tabla;
	hash_destruir_dato_t destruir_dato;
	size_t cant;
	size_t cap;
	size_t tam;
};

//Función de hashing para la estructura hash. 
//Implementación de "DJB2 by dan bernstein".
//https://gist.github.com/csthompson/6836415
size_t f_hashing(size_t tam,const char* clave){
	unsigned char* str_aux = (unsigned char*) clave;
	unsigned long hash = 5381;
	unsigned int aux;
	while((aux=*str_aux++)!=0){
		hash=((hash<<5)+hash)+aux;
	}
	return hash%tam;
}

/*****************************************
 *      FUNCIONES AUXILIARES HASH		 *
 * ***************************************/
 
//Funcion auxiliar para redimensionar la tabla de hash
bool hash_redimensionar(hash_t* hash,size_t nuevo_tam){
	lista_t** tabla_anterior = hash->tabla;
	size_t tam_anterior = hash->tam;
	lista_t** tabla_nueva = malloc(nuevo_tam*sizeof(lista_t*));
	if(tabla_nueva == NULL) return false;
	for(size_t i = 0;i<nuevo_tam;i++){
		tabla_nueva[i]=lista_crear();
		if(tabla_nueva[i]==NULL){
			for(size_t j=0;j<i;j++){
				lista_destruir(tabla_nueva[j],NULL);
				free(tabla_nueva);
			}
			return false;
		}
	}
	hash->tabla = tabla_nueva;
	hash->cant = 0;
	hash->cap = nuevo_tam*FACTOR_CAP;
	hash->tam = nuevo_tam;
	for(size_t i = 0;i<tam_anterior; i++){
		if(tabla_anterior[i]!=NULL){
			while(!lista_esta_vacia(tabla_anterior[i])){
				hash_campo_t* campo = lista_borrar_primero(tabla_anterior[i]);
				hash_guardar(hash,campo->clave,campo->dato);
				hash_campo_destruir(campo,NULL);
			}
			lista_destruir(tabla_anterior[i],NULL);
		}
	}
	free(tabla_anterior);
	return true;
}

//Si se encuentra la clave, se devuelve un iterador de lista al campo.
//Caso contrario devuelve NULL.
lista_iter_t* lista_buscar_iter_clave(const hash_t* hash, const char* clave){
	size_t pos = f_hashing(hash->tam,clave);
	if(lista_esta_vacia(hash->tabla[pos])) return NULL;
	lista_iter_t* iter = lista_iter_crear(hash->tabla[pos]);
	while(!lista_iter_al_final(iter)){
		hash_campo_t* campo = lista_iter_ver_actual(iter);
		if(strcmp(campo->clave,clave)==0){
			return iter;
		}
		lista_iter_avanzar(iter);
	}
	lista_iter_destruir(iter);
	return NULL;
}

//Crea un hash. 
hash_t* hash_crear(hash_destruir_dato_t destruir_dato){
	hash_t* hash = malloc(sizeof(hash_t));
	if(hash==NULL) return NULL;
	hash->cant = 0;
	hash->cap = TAM_INICIAL*FACTOR_CAP;
	hash->tam = TAM_INICIAL;
	hash->tabla = malloc(TAM_INICIAL*sizeof(lista_t*));
	if(hash->tabla == NULL){
		free(hash);
		return NULL;
	}
	for(size_t i = 0; i<TAM_INICIAL;i++){
		hash->tabla[i]=lista_crear();
		if(hash->tabla[i] == NULL) hash_destruir(hash);
	}
	hash->destruir_dato = destruir_dato;
	return hash;
}

//Guarda un elemento en el hash según su clave.
bool hash_guardar(hash_t *hash, const char *clave, void *dato){
	size_t pos = f_hashing(hash->tam,clave);
	bool guardado = false;
	lista_iter_t* iter = lista_buscar_iter_clave(hash,clave);
	if(iter!=NULL){
		hash_campo_t* campo = lista_iter_ver_actual(iter);
		lista_iter_destruir(iter);
		if(hash->destruir_dato!=NULL) hash->destruir_dato(campo->dato);
		campo->dato=dato;
		guardado = true;
	}else{
		hash_campo_t* nuevo_campo = hash_campo_crear(clave,dato);
		if(nuevo_campo==NULL)return false;
		guardado = lista_insertar_primero(hash->tabla[pos],nuevo_campo);
		hash->cant+=1;
	}
	if(guardado&&hash->cant>=hash->cap){
		hash_redimensionar(hash,hash->tam*FACTOR_REDIM);
	}
	return guardado;
}

//borra un elemento
void *hash_borrar(hash_t *hash, const char *clave){
	lista_iter_t* iter = lista_buscar_iter_clave(hash,clave);
	if(iter==NULL) return NULL;
	void* dato = NULL;
	hash_campo_t* campo = lista_iter_borrar(iter);
	lista_iter_destruir(iter);
	dato = campo->dato;
	hash_campo_destruir(campo,NULL);
	hash->cant-=1;
	size_t cap_aux = hash->cap/FACTOR_MIN_CAP;
	size_t tam_aux = hash->tam/FACTOR_REDIM;
	if(hash->cant<=cap_aux&&tam_aux>=TAM_INICIAL){
		hash_redimensionar(hash,tam_aux);
	}
	return dato;
}

//Obtiene el valor de una clave en el hash.
void *hash_obtener(const hash_t *hash, const char *clave){
	lista_iter_t* iter = lista_buscar_iter_clave(hash,clave);
	if(iter==NULL) return NULL;
	hash_campo_t* campo = lista_iter_ver_actual(iter);
	lista_iter_destruir(iter);
	return campo->dato;
}

//Devuelve si una clave está o no en el hash. 
bool hash_pertenece(const hash_t *hash, const char *clave){
	lista_iter_t* iter = lista_buscar_iter_clave(hash,clave);
	if(iter==NULL) {
		lista_iter_destruir(iter);
		return false;
	}
	lista_iter_destruir(iter);
	return true;
}

//Obtiene la cantidad de elementos en el hash.
size_t hash_cantidad(const hash_t *hash){
	return hash->cant;
}

//Destruye el hash. 
void hash_destruir(hash_t *hash){
	for(size_t i = 0;i<hash->tam;i++){
		while(!lista_esta_vacia(hash->tabla[i])){
			hash_campo_t* campo = lista_borrar_primero(hash->tabla[i]);
			hash_campo_destruir(campo,hash->destruir_dato);
		}
		lista_destruir(hash->tabla[i],NULL);
	}
	free(hash->tabla);
	free(hash);
}

/*******************************
 *         ITERADOR			   *
 * *****************************/
 
struct hash_iter{
	size_t pos_hash;
	const hash_t* hash;
	lista_iter_t* pos_lista;
};
 
//Crea iterador
hash_iter_t* hash_iter_crear(const hash_t* hash){
	hash_iter_t* iter = malloc(sizeof(hash_iter_t));
	if(iter==NULL) return NULL;
	iter->pos_hash = 0;
	iter->hash = hash;
	while(iter->pos_hash < iter->hash->tam){
		if(lista_esta_vacia(iter->hash->tabla[iter->pos_hash])){
			iter->pos_hash+=1;
		}else{
			break;
		}
	}
	if(iter->pos_hash<iter->hash->tam){
		iter->pos_lista=lista_iter_crear(iter->hash->tabla[iter->pos_hash]);
	}else{
		iter->pos_lista = NULL;
	}
	return iter;
}

//Avanza iterador una posición. 
bool hash_iter_avanzar(hash_iter_t* iter){
	if(hash_iter_al_final(iter))return false;
	lista_iter_avanzar(iter->pos_lista);
	if(lista_iter_al_final(iter->pos_lista)){
		lista_iter_destruir(iter->pos_lista);
		iter->pos_lista=NULL;
		iter->pos_hash+=1;
		while(!hash_iter_al_final(iter)){
			if(lista_esta_vacia(iter->hash->tabla[iter->pos_hash])){
				iter->pos_hash+=1;
			}else{
				iter->pos_lista=lista_iter_crear(iter->hash->tabla[iter->pos_hash]);
				return true;
			}
		}
		return false;
	}
	return true;
}

//Devuelve clave actual
const char *hash_iter_ver_actual(const hash_iter_t *iter){
	if(hash_iter_al_final(iter)) return NULL;
	hash_campo_t* campo = lista_iter_ver_actual(iter->pos_lista);
	return campo->clave;
}

//Comprueba si terminó la iteración
bool hash_iter_al_final(const hash_iter_t *iter){
	if(iter->pos_hash == iter->hash->tam&&iter->pos_lista==NULL) return true;
	return false;
}

//Destruye iterador
void hash_iter_destruir(hash_iter_t* iter){
	if(iter->pos_lista!=NULL)lista_iter_destruir(iter->pos_lista);
	free(iter);
}
