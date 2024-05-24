#define MAX_CLIENTS 2

#define _MAP_ROW 4
#define _MAP_COL 4
#define MAP_ROW _MAP_ROW + 1
#define MAP_COL _MAP_COL + 1
#define MAP_SIZE = MAP_COL*MAP_ROW

const int MAX_SCORE = 4; // Item max score
const int SETTING_PERIOD = 20; //Boradcast & Item generation period
const int INITIAL_ITEM = 10; //Initial number of item
const int INITIAL_BOMB = 4; //The number of bomb for each user
const int SCORE_DEDUCTION = 2; //The amount of score deduction due to bomb

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
void setItem(DGIST* dPtr);
void* printMap(void* arg); 
void* handleItem(void* arg);
void printPlayer(void* arg);

//이거 구현을 조교몬이 한 거라서 (조교몬의 피땀눈물...) 오작동이 있을 수 있어요.
//오류가 발생한 상황과 오류 내역을 lhyzone@dgist.ac.kr로 보내주면 수정해줄게요.
//오류를 수정하면 안내해드릴테니 git pull을 이용해서 항상 최신상태로 유지하세요.
