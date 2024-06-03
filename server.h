#define MAX_CLIENTS 2

#define _MAP_ROW 4
#define _MAP_COL 4
#define MAP_ROW _MAP_ROW + 1
#define MAP_COL _MAP_COL + 1
#define MAP_SIZE = MAP_COL*MAP_ROW
#define TABLE_SIZE 100

const int MAX_SCORE = 4; // Item max score
const int SETTING_PERIOD = 20; //Boradcast & Item generation period
const int INITIAL_ITEM = 10; //Initial number of item
const int INITIAL_BOMB = 4; //The number of bomb for each user
const int SCORE_DEDUCTION = 8; //The amount of score deduction due to bomb

//섹션1 서버가 여러분에게 주는 구조체에요.

//여기서 row, col을 통해 상대방의 위치 정보를 알 수 있어요.
//만약 전략을 설정하는데 상대방의 점수와 trap 개수가 필요하다면 score, bomb을 통해 알 수 있어요.
typedef struct{
    int socket;
    struct sockaddr_in address;
	int row;
	int col;
	int score;
	int bomb;
} client_info;

//nothing은 아무것도 없는 상태에요.
//item은 item이 있는 상태에요.
//trap은 trap이 있는 상태에요.
enum Status{
	nothing, //0
	item, //1
	trap //2
};

typedef struct{
	enum Status status;
	int score;
} Item;

//이 구조체의 row, col을 통해서 위치를, item의 status와 score를 통해 아이템이 있는지, trap이 있는지 판별하세요.
typedef struct {
	int row;
	int col;
	Item item; 
} Node;

//여러분은 서버에서 이 구조체를 받아올거에요. 
//players는 여러분과 상대방의 정보가 들어있는데 이 중에서 상대방의 정보만 잘 골라서 얻어야 해요.
//map은 전체 게임 map이 들어가있어요. Node는 intersection(교차점을 의미해요)
typedef struct{
	client_info players[MAX_CLIENTS];
	Node map[MAP_ROW][MAP_COL];
} DGIST;

//섹션2 여러분이 서버에게 주어야 하는 구조체에 대한 설명이에요.

//방문한 교차점에 함정을 설치하고 싶으면 1, 그렇지 않으면 0으로 설정하면 돼요.
enum Action{
	move, //0
	setBomb, //1
};

//서버에게 소켓을 통해 전달하는 구조체에요.
//QR에서 읽어온 숫자 2개를 row, col에 넣고 위의 enum Action을 참고해서 action 값을 설정하세요.
typedef struct{
	int row;
	int col;
	enum Action action;
} ClientAction;

//밑에 있는 함수는 서버 함수라서 신경쓰지 않아도 되어요.
void* handleClient(void* arg);
void* broadcastInformation(void* arg);
int setItem(DGIST* dPtr); //zero to be initial state
void* printMap(void* arg); 
void* handleItem(void* arg);
void printPlayer(void* arg);

//이거 구현을 조교몬이 한 거라서 (조교몬의 피땀눈물...) 오작동이 있을 수 있어요.
//오류가 발생한 상황과 오류 내역을 lhyzone@dgist.ac.kr로 보내주면 수정해줄게요.
//오류를 수정하면 안내해드릴테니 git pull을 이용해서 항상 최신상태로 유지하세요.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TABLE_SIZE 100

// DictNode structure to store key-value pairs
typedef struct DictNode {
    int key;
    int value;
    struct DictNode *next;
} DictNode;

// Dictionary structure
typedef struct Dictionary {
    DictNode *buckets[TABLE_SIZE];
} Dictionary;

// Hash function
unsigned int hash(int key) {
    return key % TABLE_SIZE;
}

// Initialize dictionary
Dictionary* create_dictionary() {
    Dictionary *dict = (Dictionary*)malloc(sizeof(Dictionary));
    for (int i = 0; i < TABLE_SIZE; i++) {
        dict->buckets[i] = NULL;
    }
    return dict;
}

// Create a new node
DictNode* create_node(int key, int value) {
    DictNode *new_node = (DictNode*)malloc(sizeof(DictNode));
    new_node->key = key;
    new_node->value = value;
    new_node->next = NULL;
    return new_node;
}

// Add key-value pair to dictionary
void dictionary_add(Dictionary *dict, int key, int value) {
    unsigned int index = hash(key);
    DictNode *new_node = create_node(key, value);
    if (dict->buckets[index] == NULL) {
        dict->buckets[index] = new_node;
    } else {
        DictNode *current = dict->buckets[index];
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_node;
    }
}

// Retrieve value from dictionary
int dictionary_get(Dictionary *dict, int key) {
    unsigned int index = hash(key);
    DictNode *current = dict->buckets[index];
    while (current != NULL) {
        if (current->key == key) {
            return current->value;
        }
        current = current->next;
    }
    return -1; // Return -1 if the key does not exist
}

// Check if a key exists in the dictionary
int dictionary_key_exists(Dictionary *dict, int key) {
    unsigned int index = hash(key);
    DictNode *current = dict->buckets[index];
    while (current != NULL) {
        if (current->key == key) {
            return 1; // Key exists
        }
        current = current->next;
    }
    return 0; // Key does not exist
}

// Free the memory used by the dictionary
void free_dictionary(Dictionary *dict) {
    for (int i = 0; i < TABLE_SIZE; i++) {
        DictNode *current = dict->buckets[i];
        while (current != NULL) {
            DictNode *temp = current;
            current = current->next;
            free(temp);
        }
    }
    free(dict);
}

// Function to extract the last part of the IP address and convert it to an integer
int get_last_part_as_int(const char *ip_str) {
    // Find the last occurrence of the '.' character
    const char *last_dot = strrchr(ip_str, '.');
    if (last_dot == NULL) {
        // If there is no dot in the string, return an error code, for example, -1
        return -1;
    }

    // Move the pointer past the dot
    last_dot++;

    // Convert the substring to an integer
    int last_part = atoi(last_dot);

    return last_part;
}

