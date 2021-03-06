#ifndef LIBDB_H_INCLUDED
#define LIBDB_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* check `man dbopen` */
#define KEY_SIZE 100
#define VALUE_SIZE 100

typedef unsigned char * Block;
struct DB;

//Мета информация базы данных
struct DBC
{
	/* Maximum on-disk file size */
	/* 512MB by default */
	//Максимальный размер файла бд,
	//иначе возвращать ошибку
	size_t db_size;
	/* Maximum chunk (node/data chunk) size */
	/* 4KB by default */
	//Размер блока в b - дереве
	size_t chunk_size;
	/* Maximum memory size */
	/* 16MB by default */
	size_t mem_size;
};

//Данные с размером
struct DBT
{
	void  *data;
	size_t size;
};

//Заголовок базы данных
struct Head
{
	//Размер файла БД
	size_t db_size;
	//Размер блока в B дереве
	size_t chunk_size;
	//Размер памяти
	size_t mem_size;
	//Смещение начала области статистики
	//(в блоках)
	int stat_offset;
	//Количество блоков статистики
	int stat_count;
	//Смещение начала области данных (в блоках)
	int data_offset;
	//Количество блоков данных
	int data_count;
	//Номер блока для главного элемента
	int root_id;
	//Параметр T B дерева
	int t;
};

struct NodeHead
{
	//Фактическое количество ключей
	int n;
	//1 - если лист, 0 - если внутренний узел
	int leaf;
	//Номер блока в массиве блоков
	int num;
};

struct NodeData
{
	int key_size;
	unsigned char key[KEY_SIZE];
	int value_size;
	unsigned char value[VALUE_SIZE];
	int c;
};

//////////////////////////////////////////////
/* avl_tree_tree for cash */
typedef struct Node Node;
struct Node
{
    int key;
    int data;
    Node *left;
    Node *right;
    int balance;
    int height;
};
//Вставка ключа key в дерево с корнем t
Node *insert(Node *t, int key, int data);
//Поиск заданного ключа
Node *find(Node *t, int key);
//Удаление ключа key из дерева t
Node *remove_tree(Node *t, int key);
//Освобождение памяти
Node* tfree(Node *root);
//Вывод дерева
void print_tree(Node *root);

///////////////////////////////////////////////

//Данные о блоке кэша
typedef struct Age Age;
struct Age
{
	int cash_id;
	Age *prev;
	Age *next;
};

struct Cash
{
	//Количество элементов
	int size;
	//Массив использования элементов
	int *use;
	//Массив id блоков
	int *num;
	//Массив ссылок на элементы возрастов
	Age **links;
	//Самый молодой элемент
	Age *start;
	//Самый старый элемент
	Age *finish;
	//Дерево поиска элементов кэша
	Node *rt;
	//Массив элементов кэша
	Block block;

	//Поиск блока
	int (* cash_search)(struct DB *db, int id, Node *nd);
	//Вставка блока в кэш
	Node * (* cash_insert)(struct DB *db, int key, int data);
	//Удаление блока из кэша
	void (* cash_delete)(struct DB *db, int id);
	//Чтение блока
	void (* cash_read)(struct DB *db, Block block, int id);
	//Запись блока
	void (* cash_write)(struct DB *db, Block block, int id);
};

int db_cash_search(struct DB *db, int id, Node *nd);
Node *db_cash_insert(struct DB *db, int key, int data);
void db_cash_delete(struct DB *db, int id);
void db_cash_read(struct DB  *db, Block block, int id);
void db_cash_write(struct DB *db, Block block, int id);
//Печать использования кэша
void print_use(struct DB *db);
//Печать кэша
void print_cash(struct DB *db);

/////////////////////////////////////////////////////////////

struct DB {
	/* Public API */
	/* Returns 0 on OK, -1 on Error */
	int (*close)(struct DB *db);
	int (*del)(struct DB *db, struct DBT *key);
	/* * * * * * * * * * * * * *
		* Returns malloc'ed data into 'struct DBT *data'.
		* Caller must free data->data. 'struct DBT *data'
		* must be alloced in
		* caller.
		* * * * * * * * * * * * * */
	int (*get)(struct DB *db, struct DBT *key, struct DBT *data);
	int (*put)(struct DB *db, struct DBT *key, struct DBT *data);
	/* For future uses - sync cached pages with disk
		* int (*sync)(const struct DB *db)
	* */

	/* Private API */
	//Файловый дескриптор базы данных
	FILE *fd;
	//Заголовок базы данных
	struct Head head;
	//Статистика использования блоков базы данных
	Block block_stat;
	//Головной элемент B дерева базы данных
	Block root;
	//Кэш
	struct Cash cash;

	//Чтение блока
	void (*block_read)(struct DB *db, Block block, int id);
	//Запись блока
	void (*block_write)(struct DB *db, Block block, int id);
	//Выделение блока
	int (*block_alloc)(struct DB *db);
	//Освобождение блока
	void (*block_free)(struct DB *db, int id);
}; /* Need for supporting multiple backends (HASH/BTREE) */

int get_n(Block block);
void set_n(Block block, int n);
int get_leaf(Block block);
void set_leaf(Block block, int leaf);
int get_num(Block block);
void set_num(Block block, int num);
int get_c(Block block, int i);
void set_c(Block block, int i, int c);
void get_data(Block block, int i, struct DBT *key, struct DBT *value);
void set_data(Block block, int i, struct DBT *key, struct DBT *value);
int cmp(struct DBT *key1, struct DBT *key2);

struct DB *dbcreate(char *file, struct DBC conf);
struct DB *dbopen  (char *file, struct DBC conf); /* Metadata in file */
int db_close(struct DB *db);
int close(struct DB *db);

void db_block_read(struct DB *db, Block block, int id);
void db_block_write(struct DB *db, Block block, int id);
int db_block_alloc(struct DB *db);
void db_block_free(struct DB *db, int id);
void db_block_free_root(struct DB *db, int id);

int db_del(struct DB *, void *, size_t);
int del(struct DB *db, struct DBT *key);
int internal_del(struct DB *db, Block block, struct DBT *key);
void del_1(struct DB *db, Block x, struct DBT *key);
void del_2a(struct DB *db, Block x, Block y, Block dop, int i);
void del_2b(struct DB *db, Block x, Block z, Block dop, int i);
void del_2c(struct DB *db, Block x, Block y, Block z, Block dop, int i);
void del_3a_l(struct DB *db, Block x, Block c, Block l, int i);
void del_3a_r(struct DB *db, Block x, Block c, Block r, int i);
void del_3b(struct DB *db, Block x, Block c, Block l, Block r, int i);

int db_get(struct DB *, void *, size_t, void **, size_t *);
int get(struct DB *db, struct DBT *key, struct DBT *data);
int internal_get(struct DB *db, Block block, struct DBT *key);

int db_put(struct DB *, void *, size_t, void * , size_t  );
int put(struct DB *db, struct DBT *key, struct DBT *data);
void b_tree_split_child(struct DB *db, Block x, int i, Block y);
void b_tree_ins_nf(struct DB *db, Block x, struct DBT *key, struct DBT *data);

void print_db(struct DB *db);
void print_block(struct DB *db, int id);

#endif
