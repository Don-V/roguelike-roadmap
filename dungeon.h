#ifndef DUNGEON_H
#define DUNGEON_H

#include "Character.h" /*TODO remove?*/
#include "Monster.h"
#include "Item.h"
#include <stdint.h>


#include "Cell_Pair.h"
#include <queue>

#define nRows 105
#define nCols 160

typedef struct Room{
  int x;
  int y;
  uint8_t width;
  uint8_t height;
}Room;

typedef struct Dungeon{
  Room* rooms;
  int num_rooms;
}Dungeon;

enum chances {
  ITEM_SPAWN,
  NOTHING
};

# ifdef __cplusplus
template <class T> void printmon(T* object);//TODO: remove C Linkage so this function can join the rest of them
void render_partial(Cell map[][nCols], int chars[][nCols], Character* monsts[], Pair start, Pair* newPos);
/*new stuff*/
void remove_items(std::vector<Item*>& items);
void addItems(Dungeon* dungeon, std::vector<Item*>&  items, int item_map[][nCols], int& num_items);
void render(int chars[][nCols], Character* monsts[], int item_map[][nCols], std::vector<Item*>& items, Pair start, Pair* newPos);
void generic_render(Cell map[][nCols], int chars[][nCols], Character* monsts[], int item_map[][nCols], std::vector<Item*>&  items, Pair start, Pair* newPos, bool complete);
void render_partial(Cell map[][nCols], int chars[][nCols], Character* monsts[], int item_map[][nCols], std::vector<Item*>& items, Pair start, Pair* newPos);
extern "C" {
# endif



int create_room(Cell map[][nCols], int x, int y, uint8_t* width, uint8_t* height);
int rand_gen(int min, int max);
int connect_rooms(Cell map[][nCols], Cell p, Cell q);
int update_cell(Cell* p, char value, unsigned char hardness);

int write_room(Cell map[][nCols], Room room);
int add_room(Dungeon* rlg, Room room);
void render_dungeon(Cell map[][nCols], int chars[][nCols], Character* monsts[]);
int getRoom(Dungeon d, int x, int y);
Pair determine_position(Room room);
void create_dungeon(Dungeon* dungeon, Cell map[][nCols], Cell room_cells[]);
void load_dungeon(FILE* dungeon_file, Dungeon* dungeon, Cell map[][nCols], char* dungeon_title, uint32_t* version, uint32_t* size_dungeon_file);

int cell_equals(void* c1, void* c2);
void BFS_impl(int dist[][nCols], Cell map[][nCols], Queue* q, Cell pc);
void Djikstra_impl(int t_dist[][nCols], Cell map[][nCols], Queue* q, Cell pc);

void print_Character(void* Character);
int char_equals(void* c1, void* c2);
Character* pc_init(Character* pc, Room room);

Pair* getInputC(Pair* target);
Pair* look_mode(Pair *target, int* control_mode);
// void addCharcters(Dungeon* dungeon, std::priority_queue<Character, std::vector<Character> >* evt, int nummon, Character* characters[], int chars[][nCols], unsigned int pace[]);
// void delete_dungeon(Dungeon* dungeon, std::priority_queue<Character, std::vector<Character> >* evt, Cell map[][nCols]);
void add_stairs(Dungeon* dungeon, Cell map[][nCols]);
void delete_Characters(Character* characters[], int num_characters);
void updateSight(Character* pc, Cell map[][nCols], int items[][nCols]);
Item** addItems(Dungeon* dungeon, Item** items, int item_map[][nCols], int* num_items);
void addCharcters(Dungeon* dungeon, Queue* evt, int nummon, Character* characters[], int chars[][nCols], unsigned int pace[]);
void* delete_items(Item** items, int& num_items);
void delete_dungeon(Dungeon* dungeon, Queue* evt, Cell map[][nCols]);

/*ncurses*/
void ncurses_init();
// void printmon(Character* Character);

void log_message(const char* message);
void nrender_dungeon(Cell map[][nCols], int chars[][nCols], Character* monsts[]);

void pc_render_partial(Cell map[][nCols], int chars[][nCols], Character* monsts[], Pair start, Pair* newPos);
// void render(int chars[][nCols], Character* monsts[], Pair start, Pair* newPos);
void render(int chars[][nCols], Character* monsts[], int item_map[][nCols], Item** items, Pair start, Pair* newPos);
void render_partial(Cell map[][nCols], int chars[][nCols], Character* monsts[], int item_map[][nCols], Item** items, Pair start, Pair* newPos);
void generic_render(Cell map[][nCols], int chars[][nCols], Character* monsts[], int item_map[][nCols], Item** items, Pair start, Pair* newPos, bool complete);
// void endgame(Dungeon* dungeon, Queue* game_queue, const char* endmessage);
unsigned int addItem(std::vector<Item*>& items, int item_map[][nCols], const Pair& position);
// void clear_queue(std::priority_queue<Character*, std::vector<Character*>, CharacterComparator>* q); //TODO remove


# ifdef __cplusplus
}
# endif

#endif
