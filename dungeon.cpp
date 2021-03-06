#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <getopt.h>
#include <sys/stat.h>
#include <stdint.h>
#include <endian.h>
#include <limits.h>
#include <unistd.h>
#include <ncurses.h>

#include "queue.h"
#include "dungeon.h"
#include "game.h"
#include "Player.h"
#include "object_parser.h"
#include "display.h"
#include "Potion.h"

int connect_rooms(Cell map[][nCols], Cell p, Cell q){
	while (p.x != q.x){
		if (map[p.y][p.x].value != 46){
		  update_cell(&map[p.y][p.x], 35, 0);
		}
		p.x = (p.x > q.x) ? p.x-1 : p.x+1;
	}
	while (p.y != q.y){
		if (map[p.y][p.x].value != 46){
		  update_cell(&map[p.y][p.x], 35, 0);
		}
		p.y = (p.y > q.y) ? p.y-1 : p.y+1;
	}
	return 1;
}

int update_cell(Cell* p, char value, unsigned char hardness){
  if (p->hardness == 255) return 1;
  p->value = value;
  p->hardness = hardness;
  return 0;
}

// void render_dungeon(Cell map[][nCols], int chars[][nCols], Character monsts[]){
//   int i = 0, j = 0;
//   for (i = 0; i < nRows; i++){
// 		for (j =0; j < nCols; j++){
// 		  if (i == 0 || j == 0 || i == (nRows - 1) || j == (nCols - 1)) putchar(' ');
// 		  else{
//   		  int temp = chars[i][j];
//   		  if (temp != -1) printf("%s",monsts[temp].value);
//   		  else putchar(map[i][j].value);
// 		  }
// 		}
// 		putchar('\n');
// 	}
// }

int write_room(Cell map[][nCols], Room room){
  int i = 0,j = 0;
  int n = room.x + room.width - 1;
  int m = room.y + room.height - 1;
  for (i = room.y; i <= m; i++){
    for (j = room.x; j <= n; j++){
      update_cell(&map[i][j], 46, 0);
    }
  }
  return 0;
}

int add_room(Dungeon* rlg, Room room){
  if ((rlg->num_rooms%50 == 0) && (rlg->num_rooms != 0)){
    int ratio = rlg->num_rooms*2;
    if (!(rlg->rooms = (Room *)realloc(rlg->rooms, ratio*sizeof(Room)))) return -1;
  }
  rlg->rooms[rlg->num_rooms].x = room.x;
  rlg->rooms[rlg->num_rooms].y = room.y;
  rlg->rooms[rlg->num_rooms].width = room.width;
  rlg->rooms[rlg->num_rooms].height = room.height;
  rlg->num_rooms++;
  return 0;
}

int create_room(Cell map[][nCols], int x, int y, uint8_t* width, uint8_t* height){//TODO change args up by just passing in a room pointer
	int i = 0, j= 0;
	//generate random length and width of room by genrating a random co-ord
	int max = nCols - 2;
	int min = x + 6;
	int end_x = rand_gen(min,max);
	max = nRows - 2;
	min = y + 4;
	int end_y = rand_gen(min,max);
	if (end_x - x > 25 || end_y - y > 15 ) return 0;//should be 0
	if (end_x == -1 || end_y == -1) return 0; //should be 0

	//check if this part of dungeon already contains a room
	for (i = y-1; i <= end_y + 2; i += end_y - y + 2){ //TODO check logic for this block and the next one
		for (j = x; j <= end_x; j++){
			if (map[i][j].value == 46) return 0;//should be 0
		}
	}
	for (i = x-1; i <= end_x + 2; i += end_x - x + 2){
		for (j = y; j <= end_y; j++){
			if (map[j][i].value == 46) return 0;//should be 0
		}
	}

	//fill room with dots
	for (j = y; j <= end_y; j++){
		for (i = x; i <= end_x; i++){
		  update_cell(&map[j][i], 46, 0);
		}
	}

	//return width and length of room respectively
	*width = end_x - x + 1;
  *height = end_y - y + 1;
	return 1;
}

int getRoom(Dungeon d, int x, int y){
  int i = 0;
  for (i = 0; i < d.num_rooms; i++){
    Room room = d.rooms[i];
    if (x >= room.x && x < (room.x + room.width) && y >= room.y && y < (room.y + room.height))
      return i;
  }
  return -1;
}

bool in_room(const Room& room, const Pair& position){
  if (position.x >= room.x && position.x < (room.x + room.width) && position.y >= room.y && position.y < (room.y + room.height))
    return true;
  return false;
}

Pair select_position(Pair curr_pos, const Room& boundary, int chars[][nCols], Character* characters[], int item_map[][nCols], std::vector<Item*>& items, Pair start, char value){ //TODO: handle -ve initial target
  value = value ? value : '^';
  do{
    Pair prev = curr_pos;
    std::string char_value;
    char_value.push_back(value);
    log_message(std::string("Choose target by moving the '") + char_value + "' and press SPACEBAR to select postion, or press ESC or q or Q to exit", 0);
    custom_render(chars, characters, item_map, items, start, 0, value, curr_pos);
    curr_pos = getInputS(&curr_pos);
    if (curr_pos.x == -10 && curr_pos.y == -10){
      curr_pos = prev;
      break;
    }else if(curr_pos.x == -11 && curr_pos.y == -11) break;
    else if (!in_room(boundary, curr_pos)) curr_pos = prev;
  }while(true);
  return curr_pos;
}

Pair determine_position(Room room){
  Pair result = {
    rand_gen(room.x, room.x + room.width - 1),
    rand_gen(room.y, room.y + room.height - 1)
  };
  return result;
}

void BFS_impl(int dist[][nCols], Cell map[][nCols], Queue* q, Cell pc){
    int i = 0,j = 0;
    uint8_t marked[nRows][nCols];
    /* Initialize distance of all cells to infinity & initialize all cells as unmarked*/
    for (i = 0; i < nRows; i++){
  		for (j =0; j < nCols; j++){
  		  dist[i][j] = INT_MAX - 100;
  			marked[i][j] = 0;
  		}
  	}
  	
    /*Create mask cells to be used with the queue*/
    Cell* temps[nRows][nCols];
    for (i = 0; i < nRows; i++){
      for (j = 0; j< nCols; j++){
        temps[i][j] = NULL;
      }
    }
  
  	/*BFS implementation with linear queue*/
    enqueue(q, &pc);
    marked[pc.y][pc.x] = 1;
    dist[pc.y][pc.x] = 0;
    while (q->size > 0){
      Cell curr = *(Cell *)peek(q, &curr);
      int x = curr.x, y = curr.y;
      dequeue(q);
  
      for (i=y-1; i <= y+1; i++){
        for (j=x-1; j <= x+1; j++){
          if (!(i== y && j == x) && (i > 0 && j > 0 && i < nRows-1 && j < nCols-1) && (map[i][j].hardness == 0)){
            if (marked[i][j] && (dist[y][x] + 1 < dist[i][j])) dist[i][j] = dist[y][x] + 1;
            else if (marked[i][j] == 0){
              temps[i][j] = (Cell *)malloc(sizeof(Cell));
              Cell temp_cell = {j, i, map[i][j].value, map[i][j].hardness};
              *temps[i][j] = temp_cell;
              enqueue(q, temps[i][j]);
              dist[i][j] = dist[y][x] + 1;
              marked[i][j] = 1;
            }
            
          }
        }
      }
    }
    
    /* Freeing mask cells that were used*/
    for (i = 0; i < nRows; i++){
      for (j = 0; j< nCols; j++){
        if (temps[i][j]){
          free(temps[i][j]);
          temps[i][j] = NULL;
        }
      }
    }
  	empty_queue(q);
  }
  
void Djikstra_impl(int t_dist[][nCols], Cell map[][nCols], Queue* q, Cell pc){
  /*Make sure queue is empty TODO: Or just use a new queue in here*/
  empty_queue(q);
  int i = 0, j = 0;
  uint8_t weight[nRows][nCols];
  uint8_t marked[nRows][nCols];
  
  /*Mask cells to be used with the queue*/
  Cell* temps[nRows][nCols];
  for (i = 0; i < nRows; i++)
    for (j = 0; j< nCols; j++)
      temps[i][j] = NULL;
  
  /*Initialize distance of every cell to infinte distances and unmarked*/
  for (i = 0; i < nRows; i++){
		for (j =0; j < nCols; j++){
		  if (map[i][j].hardness == 0) weight[i][j] = 1;
		  else if (map[i][j].hardness >= 1 && map[i][j].hardness <= 84) weight[i][j] = 1;
		  else if (map[i][j].hardness >= 85 && map[i][j].hardness <= 170) weight[i][j] = 2;
		  else if (map[i][j].hardness >= 171 && map[i][j].hardness <= 254) weight[i][j] = 3;
		  else weight[i][j] = UINT8_MAX;
			marked[i][j] = 0;
			t_dist[i][j] = INT_MAX;
		}
	}

	enqueue(q, &pc);//NOTE: In case of any warnings, pc is not type cell but is expected to evaluate type cell
	t_dist[pc.y][pc.x] = 0;
	
	/*shortest path finding using Djistra's algorithm*/
  while (q->size > 0){
    Cell* curr = (Cell *)peek(q, &curr);
    int x = curr->x, y = curr->y;
    dequeue(q);
    marked[y][x] = 1;
    for (i = y-1; i <= y+1; i++){
      for (j = x-1; j <= x+1; j++){
        if (!(i== y && j == x) && (i > 0 && j > 0 && i < nRows && j < nCols)){
          int alt_dist = t_dist[y][x] + weight[y][x];
          if (marked[i][j] == 0 && (alt_dist < t_dist[i][j])){
            t_dist[i][j] = alt_dist;
            if (temps[i][j]==NULL)
              temps[i][j] = (Cell *)malloc(sizeof(Cell));
            Cell temp_cell = {j, i, map[i][j].value, map[i][j].hardness};
            *temps[i][j] = temp_cell;
            add_with_priority(q, temps[i][j], alt_dist);
          }
        }
      }
    }
  }

  /*Free all memory malloc'd for masked cells*/
  for (i = 0; i < nRows; i++)
      for (j = 0; j< nCols; j++)
          free(temps[i][j]);
}

void nrender_dungeon(Cell map[][nCols], int chars[][nCols], Character* monsts[]){
  int i = 0, j = 0;
  move(1, 1);
  for (i = 0; i < nRows; i++){
		for (j =0; j < nCols; j++){
		  if (i == 0 || j == 0 || i == (nRows - 1) || j == (nCols - 1)) addch(' ');
		  else{
  		  int temp = chars[i][j];
  		  if (temp != -1) printmon(monsts[chars[i][j]]);
  		  else addch(map[i][j].value);
		  }
		}
		addch('\n');
	}
	refresh();
// 	move(0,0);
}

void render_partial(Cell map[][nCols], int chars[][nCols], Character* monsts[], Pair start, Pair* newPos){
  int i = 0, j = 0;
  start.x = (start.x < 0) ? 0 : start.x;
  start.y = (start.y < 0) ? 0 : start.y;
  int endRow = start.y + 21, endCol = start.x + 80;
  if (endCol > nCols - 1){
    start.x = nCols - 1 - 80;
    endCol = nCols - 1;
  }
  if (endRow > nRows - 1){
    start.y = nRows - 1 - 21;
    endRow = nRows - 1;
  }
  move(1, 0);
  for (i = start.y; i < endRow; i++){
		for (j = start.x; j < endCol; j++){
		  if (i == 0 || j == 0 || i == (nRows - 1) || j == (nCols - 1)) addch(' ');
		  else{
  		  int temp = chars[i][j];
  		  if (temp != -1) printmon(monsts[chars[i][j]]);
  		  else addch(map[i][j].value);
		  }
		}
		addch('\n');
	}
	if(newPos) *newPos = start;
	refresh();
}

void render_partial(Cell map[][nCols], int chars[][nCols], Character* monsts[], int item_map[][nCols], Item** items, Pair start, Pair* newPos){
  int i = 0, j = 0; //legacy
  start.x = (start.x < 0) ? 0 : start.x;
  start.y = (start.y < 0) ? 0 : start.y;
  int endRow = start.y + 21, endCol = start.x + 80;
  if (endCol > nCols - 1){
    start.x = nCols - 1 - 80;
    endCol = nCols - 1;
  }
  if (endRow > nRows - 1){
    start.y = nRows - 1 - 21;
    endRow = nRows - 1;
  }
  move(1, 0);
  for (i = start.y; i < endRow; i++){
		for (j = start.x; j < endCol; j++){
		  if (i == 0 || j == 0 || i == (nRows - 1) || j == (nCols - 1)) addch(' ');
		  else{
  		  int temp = chars[i][j];
  		  if (temp != -1) printmon(monsts[chars[i][j]]);
  		  else if (item_map[i][j] != -1) printmon(items[item_map[i][j]]);
  		  else addch(map[i][j].value);
		  }
		}
		addch('\n');
	}
	if(newPos) *newPos = start;
	refresh();
}

void render_partial(Cell map[][nCols], int chars[][nCols], Character* monsts[], int item_map[][nCols], std::vector<Item*>& items, Pair start, Pair* newPos){
  int i = 0, j = 0;
  start.x = (start.x < 0) ? 0 : start.x;
  start.y = (start.y < 0) ? 0 : start.y;
  int endRow = start.y + 21, endCol = start.x + 80;
  if (endCol > nCols - 1){
    start.x = nCols - 1 - 80;
    endCol = nCols - 1;
  }
  if (endRow > nRows - 1){
    start.y = nRows - 1 - 21;
    endRow = nRows - 1;
  }
  move(1, 0);
  for (i = start.y; i < endRow; i++){
		for (j = start.x; j < endCol; j++){
		  if (i == 0 || j == 0 || i == (nRows - 1) || j == (nCols - 1)) addch(' ');
		  else{
  		  int temp = chars[i][j];
  		  if (temp != -1) printmon(monsts[chars[i][j]]);
  		  else if (item_map[i][j] != -1) printmon(items[item_map[i][j]]);
  		  else {
  		    if (map[i][j].value == '$'){
  		      attron(COLOR_PAIR(COLOR_GREEN));
            addch(map[i][j].value);
            attroff(COLOR_PAIR(COLOR_GREEN));
  		    }else
  		      addch(map[i][j].value);
  		  }
		  }
		}
		addch('\n');
	}
	if(newPos) *newPos = start;
	refresh();
}

void pc_render_partial(Cell map[][nCols], int chars[][nCols], Character* monsts[], Pair start, Pair* newPos){
  int** sight = csetSight(monsts[0], nRows, nCols); /*TODO: check value of sight*/
  int i = 0, j = 0;
  start.x = (start.x < 0) ? 0 : start.x;
  start.y = (start.y < 0) ? 0 : start.y;
  int endRow = start.y + 21, endCol = start.x + 80;
  if (endCol > nCols - 1){
    start.x = nCols - 1 - 80;
    endCol = nCols - 1;
  }
  if (endRow > nRows - 1){
    start.y = nRows - 1 - 21;
    endRow = nRows - 1;
  }
  move(1, 0);
  for (i = start.y; i < endRow; i++){
		for (j = start.x; j < endCol; j++){
		  if (i == 0 || j == 0 || i == (nRows - 1) || j == (nCols - 1)) addch(' ');
		  else if (sight[i][j] != -1){
  		  int temp = chars[i][j];
  		  int in_range = (j <= cgetX(monsts[0]) + 5) && (j >= cgetX(monsts[0]) - 5) && (i <= cgetY(monsts[0]) + 5) && (i >= cgetY(monsts[0]) - 5);
  		  if ((temp != -1) && (in_range)) printmon(monsts[chars[i][j]]); /*There is a monster on the terrain and the monster is within range*/
  		  else addch(sight[i][j]);
		  }else addch(' ');
		}
		addch('\n');
	}
	if(newPos) *newPos = start;
	refresh();
}

void generic_render(Cell map[][nCols], int chars[][nCols], Character* monsts[], int item_map[][nCols], Item** items, Pair start, Pair* newPos, bool complete){
  if (complete) //legacy
    render_partial(map, chars, monsts, item_map, items, start, newPos);
  else
    render(chars, monsts, item_map, items, start, newPos);
}

void generic_render(Cell map[][nCols], int chars[][nCols], Character* monsts[], int item_map[][nCols], std::vector<Item*>&  items, Pair start, Pair* newPos, bool complete){
  if (complete)
    render_partial(map, chars, monsts, item_map, items, start, newPos);
  else
    render(chars, monsts, item_map, items, start, newPos);
}

void render(int chars[][nCols], Character* monsts[], int item_map[][nCols], Item** items, Pair start, Pair* newPos){ //legacy
  int** sight = csetSight(monsts[0], nRows, nCols); /*TODO: check value of sight*/
  int i = 0, j = 0;
  start.x = (start.x < 0) ? 0 : start.x;
  start.y = (start.y < 0) ? 0 : start.y;
  int endRow = start.y + 21, endCol = start.x + 80;
  if (endCol > nCols - 1){
    start.x = nCols - 1 - 80;
    endCol = nCols - 1;
  }
  if (endRow > nRows - 1){
    start.y = nRows - 1 - 21;
    endRow = nRows - 1;
  }
  move(1, 0);
  for (i = start.y; i < endRow; i++){
		for (j = start.x; j < endCol; j++){
		  if (i == 0 || j == 0 || i == (nRows - 1) || j == (nCols - 1)) addch(' ');
		  else if (sight[i][j] != 0){ /*PC can see/has seen this spot*/
  		  int temp = chars[i][j];
  		  int in_range = (j <= cgetX(monsts[0]) + 5) && (j >= cgetX(monsts[0]) - 5) && (i <= cgetY(monsts[0]) + 5) && (i >= cgetY(monsts[0]) - 5);
  		  if ((temp != -1) && (in_range)) printmon(monsts[chars[i][j]]); /*There is a monster on the terrain and the monster is within range*/
  		  else {
  		    //TODO: check for item
  		    if (/*sight[i][j] != '.' && sight[i][j] != '#' && sight[i][j] != ' ' && sight[i][j] != '>' && sight[i][j] != '<'*/sight[i][j] < 0){ //TODO: use a function that checks
  		    int fog_item = sight[i][j] * -1;
  		      printmon(items[fog_item]);
  		    }else
  		      addch(sight[i][j]);
  		  }
		  }else addch(' ');
		}
		addch('\n');
	}
	if(newPos) *newPos = start;
	refresh();
}

void render(int chars[][nCols], Character* monsts[], int item_map[][nCols], std::vector<Item*>& items, Pair start, Pair* newPos){
  int** sight = csetSight(monsts[0], nRows, nCols); /*TODO: check value of sight*/
  int i = 0, j = 0;
  start.x = (start.x < 0) ? 0 : start.x;
  start.y = (start.y < 0) ? 0 : start.y;
  int endRow = start.y + 21, endCol = start.x + 80;
  if (endCol > nCols - 1){
    start.x = nCols - 1 - 80;
    endCol = nCols - 1;
  }
  if (endRow > nRows - 1){
    start.y = nRows - 1 - 21;
    endRow = nRows - 1;
  }
  move(1, 0);
  for (i = start.y; i < endRow; i++){
		for (j = start.x; j < endCol; j++){
		  if (i == 0 || j == 0 || i == (nRows - 1) || j == (nCols - 1)) addch(' ');
		  else if (sight[i][j] != 0){ /*PC can see/has seen this spot*/
  		  int temp = chars[i][j];
  		  int in_range = (j <= cgetX(monsts[0]) + 5) && (j >= cgetX(monsts[0]) - 5) && (i <= cgetY(monsts[0]) + 5) && (i >= cgetY(monsts[0]) - 5);
  		  if ((temp != -1) && (in_range)) printmon(monsts[chars[i][j]]); /*There is a monster on the terrain and the monster is within range*/
  		  else {
  		    //TODO: check for item
  		    if (sight[i][j] < 0){
  		      int fog_item = sight[i][j] * -1;
  		      printmon(items[fog_item]);
  		    }else{
    		    if (sight[i][j] == '$'){
    		      attron(COLOR_PAIR(COLOR_GREEN));
              addch(sight[i][j]);
              attroff(COLOR_PAIR(COLOR_GREEN));
    		    }else
    		      addch(sight[i][j]);
  		     }
  		  }
		  }else addch(' ');
		}
		addch('\n');
	}
	if(newPos) *newPos = start;
	refresh();
}

void custom_render(int chars[][nCols], Character* monsts[], int item_map[][nCols], std::vector<Item*>& items, Pair start, Pair* newPos, char value, const Pair& curr){
  int** sight = csetSight(monsts[0], nRows, nCols); /*TODO: check value of sight*/
  int i = 0, j = 0;
  start.x = (start.x < 0) ? 0 : start.x;
  start.y = (start.y < 0) ? 0 : start.y;
  int endRow = start.y + 21, endCol = start.x + 80;
  if (endCol > nCols - 1){
    start.x = nCols - 1 - 80;
    endCol = nCols - 1;
  }
  if (endRow > nRows - 1){
    start.y = nRows - 1 - 21;
    endRow = nRows - 1;
  }
  move(1, 0);
  for (i = start.y; i < endRow; i++){
		for (j = start.x; j < endCol; j++){
		  if (i == 0 || j == 0 || i == (nRows - 1) || j == (nCols - 1)) addch(' ');
		  else if (curr.x == j && curr.y == i) addch(value);
		  else if (sight[i][j] != 0){ /*PC can see/has seen this spot*/
  		  int temp = chars[i][j];
  		  int in_range = (j <= cgetX(monsts[0]) + 5) && (j >= cgetX(monsts[0]) - 5) && (i <= cgetY(monsts[0]) + 5) && (i >= cgetY(monsts[0]) - 5);
  		  if ((temp != -1) && (in_range)) printmon(monsts[chars[i][j]]); /*There is a monster on the terrain and the monster is within range*/
  		  else {
  		    //TODO: check for item
  		    if (sight[i][j] < 0){ //TODO: use a function that checks
  		    int fog_item = sight[i][j] * -1;
  		      printmon(items[fog_item]);
  		    }else{
    		    if (sight[i][j] == '$'){
    		      attron(COLOR_PAIR(COLOR_GREEN));
              addch(sight[i][j]);
              attroff(COLOR_PAIR(COLOR_GREEN));
    		    }else
    		      addch(sight[i][j]);
  		     }
  		  }
		  }else addch(' ');
		}
		addch('\n');
	}
	if(newPos) *newPos = start;
	refresh();
}


template <class T>
void printmon(T* const object){
  attron(COLOR_PAIR(object->getColor()));
  addch(object->getSymbol());
  attroff(COLOR_PAIR(object->getColor()));
}

// void addCharcters(Dungeon* dungeon, Queue* evt, int nummon, Character* characters[], int chars[][nCols], unsigned int pace[]){
//   int i;
//   memset(chars, -1, sizeof(int)*nRows*nCols);
//   characters[0] = pc_init(characters[0], dungeon->rooms[0]);
//   for(i = 0; i < nummon + 1; i++){
//     /*Initialize monsters*/
//     if (i != 0){
//       int rand_room = rand_gen(1, dungeon->num_rooms - 1); //This makes sure no monster is spawned int he same room as the PC.
//       uint8_t type = rand() & 0xF;//rand_gen(0x0,0xF);
//       char temp_val[2];
//       sprintf(temp_val, "%x", type);
//       characters[i] = new Monster(
//         i, /*id*/
//         rand_gen(dungeon->rooms[rand_room].x, dungeon->rooms[rand_room].x + dungeon->rooms[rand_room].width - 1), /*x-position*/
//         rand_gen(dungeon->rooms[rand_room].y, dungeon->rooms[rand_room].y + dungeon->rooms[rand_room].height - 1),/*y-position*/
//         rand_gen(5, 20), /*speed*/
//         type /*type*/
//       );
//     }
//     chars[cgetY(characters[i])][cgetX(characters[i])] = i;
//     pace[i] = 1000/cgetSpeed(characters[i]);
//     add_with_priority(evt, characters[i], pace[i]);
//   }
//   // characters[0].speed = 0xF; //not too sure why i do this, i know it blocks pc from getting assigned a value execpt this value is hanged by new dungeon generation
// }

void addCharcters(Dungeon* dungeon, Queue* evt, int nummon, Character* characters[], int chars[][nCols], unsigned int pace[]){
  int i;
  memset(chars, -1, sizeof(int)*nRows*nCols);
  characters[0] = pc_init(characters[0], dungeon->rooms[0]);
  for(i = 0; i < nummon + 1; i++){
    /*Initialize monsters*/
    if (i != 0){
      int rand_room = rand_gen(1, dungeon->num_rooms - 1); //This makes sure no monster is spawned int he same room as the PC.
      characters[i] = new Monster(
        i, /*id*/
        rand_gen(dungeon->rooms[rand_room].x, dungeon->rooms[rand_room].x + dungeon->rooms[rand_room].width - 1), /*x-position*/
        rand_gen(dungeon->rooms[rand_room].y, dungeon->rooms[rand_room].y + dungeon->rooms[rand_room].height - 1),/*y-position*/
        object_parser::getCompleteMonsterStub(rand_gen(0, object_parser::getNumMonstubs() - 1)) /*monster stub ranging from 0 to stubs - 1*/
      );
    }
    chars[cgetY(characters[i])][cgetX(characters[i])] = i;
    pace[i] = 1000/cgetSpeed(characters[i]);
    add_with_priority(evt, characters[i], pace[i]);
  }
  // characters[0].speed = 0xF; //not too sure why i do this, i know it blocks pc from getting assigned a value execpt this value is hanged by new dungeon generation
}

Item** addItems(Dungeon* dungeon, Item** items, int item_map[][nCols], int* num_items){ //legacy
  memset(item_map, -1, sizeof(int)*nRows*nCols);
  *num_items = (*num_items < 0) ? *num_items * -1 : rand_gen(20, dungeon->num_rooms * 2);
  items = (Item**)malloc(sizeof(Item*) * *num_items);
  for (int i = 0; i < *num_items; i++){
    Pair co_ords = determine_position(dungeon->rooms[rand_gen(0, dungeon->num_rooms - 1)]); /*get position from random room*/
    int jaja = rand_gen(0, object_parser::getNumItemstubs() - 1);
    items[i] = new Item(
      i,
      object_parser::getCompleteItemStub(jaja)
    );
    if (item_map[co_ords.y][co_ords.x] != -1){
      items[i]->stack(items[item_map[co_ords.y][co_ords.x]]);
    }
    item_map[co_ords.y][co_ords.x] = i;
  }
  return items;
}

void addItems(Dungeon* dungeon, std::vector<Item*>& items, int item_map[][nCols], int& num_items){
  memset(item_map, -1, sizeof(int)*nRows*nCols);
  num_items = (num_items < 0) ? num_items * -1 : rand_gen(20, dungeon->num_rooms * 2);
  unsigned int prev_size = items.size();
  for (std::vector<int>::size_type i = prev_size; i < prev_size + (unsigned int)num_items; i++){
  // for (int i = 0; i < num_items; i++){
    Pair co_ords = determine_position(dungeon->rooms[rand_gen(0, dungeon->num_rooms - 1)]); /*get position from random room*/
    // items.push_back(new Item( //TODO emplace?
    //   i,
    //   object_parser::getCompleteItemStub(rand_gen(0, object_parser::getNumItemstubs() - 1))
    // ));
    addItem(i, items);
    if (item_map[co_ords.y][co_ords.x] != -1){
      items[i]->stack(items[item_map[co_ords.y][co_ords.x]]);
    }
    item_map[co_ords.y][co_ords.x] = i;
  }
}

unsigned int addItem(int id, std::vector<Item*>& items){
  unsigned int new_id = (id >= 0) ? (unsigned int)id : items.size();
  auto stub = object_parser::getCompleteItemStub(rand_gen(0, object_parser::getNumItemstubs() - 1));
  Item* item = 0;
  if (stub->isKind("potion"))
    item = new Potion(id, stub);
  else item = new Item(id, stub);
  // Item* item = stub->isKind("potion") ? new Potion(id, stub) : new Item(id, stub);
  items.push_back(item);
  return new_id;
}

Character* pc_init(Character* pc, Room room){
  int pc_x = rand_gen(room.x, room.x + room.width - 1);
  int pc_y = rand_gen(room.y, room.y + room.height - 1);
  pc = Player::getPlayer();
  pc->setPos(&pc_x, &pc_y);
  int** sight = csetSight(pc, nRows, nCols); /*TODO: check value of sight*/
  for (int i = 0; i < nRows; i++){
    for (int j = 0; j < nCols; j++){
      sight[i][j] = 0;
    }
  }
  return pc;
}

void updateSight(Character* pc, Cell map[][nCols], int items[][nCols]){ /*TODO: move to Character class*/
  int x = cgetX(pc), y = cgetY(pc);
  int i,j;
  int** sight = csetSight(pc, nRows, nCols); /*TODO: check value of sight*/
  Pair start = {(x - 5 > 1) ? x - 5 : 1, (y - 5 > 1) ? y - 5 : 1};
  Pair end = {(x + 5 < nCols - 1) ? x + 5 : nCols - 2, (y + 5 < nRows - 1) ? y + 5 : nRows - 2};
  for (i = start.y; i <= end.y; i++){
    for (j = start.x; j <= end.x; j++){
      sight[i][j] = (items[i][j] == -1) ? map[i][j].value : -1 * items[i][j]; //TODO:(-2 means item) instead of checking those different values, you can set this one to a constant value that always means object on this position: NO two monsters on the same spot
    }
  }
}

void create_dungeon(Dungeon* dungeon, Cell map[][nCols], Cell room_cells[]){
  int i, count = 0, tries = 0, area = 0;
  dungeon->rooms = (Room *)malloc(sizeof(Room)*50);
  while ((area < .2*(nRows*nCols) || count < 10) && (++tries < 10000)){
    Room room; // = {0, 0, 0, 0};
		room.y = rand()%(nRows-2) + 1;
		room.x = rand()%(nCols-2) + 1;
		
		if (!create_room(map, room.x, room.y, &room.width, &room.height)) continue; //continue loop if room couldn't be created
		area += room.width * room.height;
		
    //get random cell in each room
		room_cells[count].x = rand_gen(room.x, room.width + room.x -1);
		room_cells[count].y = rand_gen(room.y, room.height + room.y -1);
		count++;
		
		//add room to rooms array TODO merge this with code above
		if (add_room(dungeon, room) == -1){
		  count--;
		}
	}
    //connect random cells
  	for (i = 0; i < count - 1; i++){
  		connect_rooms(map, room_cells[i], room_cells[i+1]);
    }
}

void load_dungeon(FILE* dungeon_file, Dungeon* dungeon, Cell map[][nCols], char* dungeon_title, uint32_t* version, uint32_t* size_dungeon_file){
  int i,j;
  uint32_t temp = 0;
  //read dungeon title
  char temp_name[13];
  temp_name[12] = 0;
  fread(temp_name, 12, 1, dungeon_file); //cs: 12
  strcpy(dungeon_title, temp_name);
  
  //read verison
  fread(&temp, 4, 1, dungeon_file); //cs: 4
  *version = be32toh(temp);
  
  //read size of file
  fread(&temp, 4, 1, dungeon_file); //cs:4
  *size_dungeon_file = be32toh(temp);
  
  //read hardness
  for (i = 0; i < nRows; i++)
		for (j =0; j < nCols; j++)
		  fread(&(map[i][j].hardness), sizeof(unsigned char), 1, dungeon_file); //cs:8
  
  //quickly write corridors
  for (i = 0; i < nRows; i++)
  		for (j =0; j < nCols; j++)
  			if (map[i][j].hardness == 0) update_cell(&map[i][j], 35, 0);
  
  //read rooms
  // int put = 0;
  Room room = {0,0,0,0};
  dungeon->rooms = (Room *)malloc(sizeof(Room)*50);
  while((fread(&room.x, sizeof(uint8_t), 1, dungeon_file)) == 1){
    fread(&room.y, sizeof(uint8_t), 1, dungeon_file);
    fread(&room.width, sizeof(uint8_t), 1, dungeon_file);
    fread(&room.height, sizeof(uint8_t), 1, dungeon_file);
    write_room(map, room);
    add_room(dungeon, room);
  }
  
  //display corridor
  fclose(dungeon_file);
}

/*This function assumes that there are at least 2 rooms*/
void add_stairs(Dungeon* dungeon, Cell map[][nCols]){
  int i;
	int n_stairs = rand_gen(2,4);
	for(i = 0; i < n_stairs; i++){
	  /*create upstair in random room then downstair in the next room */
	  int room_no = rand_gen(0, dungeon->num_rooms - 2);
	  Pair pos = determine_position(dungeon->rooms[room_no]);
	  map[pos.y][pos.x].value = '<';
	  pos = determine_position(dungeon->rooms[room_no + 1]);
	  map[pos.y][pos.x].value = '>';
	}
}

void add_shops(Dungeon* dungeon, Cell map[][nCols]){
  int i;
	int n_stairs = rand_gen(2,4);
	for(i = 0; i < n_stairs; i++){
	  /*create upstair in random room then downstair in the next room */
	  int room_no = rand_gen(0, dungeon->num_rooms - 2);
	  Pair pos = determine_position(dungeon->rooms[room_no]);
	  map[pos.y][pos.x].value = '$';
	  pos = determine_position(dungeon->rooms[room_no + 1]);
	  map[pos.y][pos.x].value = '$';
	}
}

void delete_dungeon(Dungeon* dungeon, Queue* evt, Cell map[][nCols]){
  int i,j;
  /*free dungeon rooms*/
  free(dungeon->rooms);
  dungeon->num_rooms = 0;
  /*free queue*/
  empty_queue(evt);
  /*Reset map*/
  for (i = 0; i < nRows; i++){
		for (j =0; j < nCols; j++){
			map[i][j].value = 32;
			map[i][j].hardness = (i == 0 || j == 0 || i == nRows-1 || j == nCols-1) ? 255 : rand_gen(1,254);
		}
	}
	
}

void* delete_items(Item** items, int& num_items){ //legacy
  if (!items) return 0;
  for (int i = 0; i < num_items; i++){
    delete items[i];
    items[i] = 0;
  }
  free(items);
  items = 0;
  return items;
}

void remove_items(std::vector<Item*>& items){
  // if (!items) return 0;
    /*for (std::vector<int>::size_type i = 0; i < items.size(); i++){
      if (!items[i]->equiped()){ 
        delete items[i];
        items.erase(items.begin() + i);
      }
    }*/
    std::vector<int>::size_type i = 0;
    while(i != items.size()){
      if (!items[i]->isEquiped()){
        delete items[i];
        items.erase(items.begin() + i);
      }else i++;
    }
    for (std::vector<int>::size_type j = 0; j < items.size(); j++){
      items[j]->resetId(j);
    }
}

void clearShop(std::vector<Item*>& wShop){
  for (auto item_p : wShop)
    delete item_p;
  wShop.clear();
}

bool isRanged(Item* item){
  return std::string(item->getType()) == "RANGED";
}

void item_drop(std::vector<Item*>& items, int chance, int item_map[][nCols], const Pair& target){
  if (probability(chance)){
    uint32_t spawn_id = addItem(-1, items);
    if (item_map[target.y][target.x] != -1){
      items[spawn_id]->stack(items[item_map[target.y][target.x]]);
    }
    item_map[target.y][target.x] = spawn_id;
  }
}