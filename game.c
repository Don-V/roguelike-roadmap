#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <getopt.h>
#include <sys/stat.h>
#include <stdint.h>
#include <endian.h> //TODO make sure you're using the functions right
#include <limits.h> //TODO might not need

#define nRows 105
#define nCols 160

typedef struct Point{
	int x;
	int y;
	char value;
	unsigned char hardness;
}Point;

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


int create_room(Point map[][nCols], int x, int y, uint8_t* width, uint8_t* height);
int rand_gen(int min, int max);
int connect_rooms(Point map[][nCols], Point p, Point q);
int updatePoint(Point* p, char value);
void render(Point map[][nCols]);
int write_room(Point map[][nCols], Room room);
int addRoom(Dungeon* rlg, Room room);

int main(int argc, char *argv[]){
  
  //regular
  Dungeon dungeon;
  dungeon.rooms = malloc(sizeof(Room)*2500); //debug this issue
  dungeon.num_rooms = 0;
  int area = 0, tries = 0, count = 0;
	Point map[nRows][nCols];
	int i = 0, j = 0;
	Point room_points[50];
  
  //dungeon
  FILE *dungeon_file;
  char dungoen_title[20];
  uint32_t version;
  uint32_t size_dungeon_file = 0;
  
  //handle options an set seed
  int longindex; //TODO remove this and set the args part to NULL
  int option, load = 0, save = 0, seed = (unsigned) time(NULL);
  char* load_file;
  struct option longopts[] = {
    {"load", optional_argument, NULL, 'l'},
    {"save", no_argument, &save, 1},
    {"seed", optional_argument, NULL, 'e'},
    {0,0,0,0}
  };
  while ((option = getopt_long(argc, argv, ":e:", longopts, &longindex)) != -1){
    switch(option){
      case 'l':
        load = 1;
        if (optarg) load_file = optarg;
        else {
          load_file = getenv("HOME");
          strcat(load_file, "/.rlg327/dungeon");
        }
        break;
      case 'e':
        if (optarg) seed = atoi(optarg);
        else fprintf(stderr, "%s: option seed requires an argument\n", argv[0]);;
        break;
      case 0:
        break;
      // case 1:
      //   fprintf(stderr, "%s: unexpected argument %s", argv[0], optarg);
      //   break;
      case '?':
          fprintf(stderr, "%s: Invalid option: %c ignored\n", argv[0], optopt);
        break;
      case ':':
        fprintf(stderr, "%s: option -%c requires an argument\n", argv[0], optopt);
        break;
      default:
        fprintf(stderr, "%s: option -%c is ignored because it is invalid\n", argv[0], optopt);
        
    }
  }
  
  //seed
  srand(seed);
  
  //initialize dungoen
  for (i = 0; i < nRows; i++){
		for (j =0; j < nCols; j++){
			map[i][j].value = 32;
			map[i][j].hardness = (i == 0 || j == 0 || i == nRows-1 || j == nCols-1) ? 255 : 0;
		}
	}
  
  // printf("HOME : %s\n", getenv("HOME")); TODO remove
  if (load){
    if (!(dungeon_file = fopen(load_file, "r"))){ //TODO change test to path to file in {HOME}/.rlg327
      fprintf(stderr, "The file couldn't be opened\n"); //TODO add name of file(path maybe)
      return 1;
    }else{
      uint32_t temp = 0;//TODO remove all printfs
      printf("Started loading\n");
      //read dungeon title
      char* temp_name = malloc(sizeof(char)*13);
      temp_name[12] = 0;//TODO might not be necessary
      fread(temp_name, 12, 1, dungeon_file); //cs: 12
      strcpy(dungoen_title, temp_name); //TODO figure out dungeon title toendian
      printf("dungeon title: %s\n", dungoen_title);
      free(temp_name);
      
      //read verison
      fread(&temp, 4, 1, dungeon_file); //cs: 4
      version = be32toh(temp);
      printf("version: %d\n", version);
      
      //read size of file
      fread(&temp, 4, 1, dungeon_file); //cs:4
      size_dungeon_file = be32toh(temp);
      printf("size_dungeon_file: %d\n", size_dungeon_file);
      
      //read hardness
      // int temper = 20;
      for (i = 0; i < nRows; i++){
    		for (j =0; j < nCols; j++){
    		  fread(&(map[i][j].hardness), sizeof(unsigned char), 1, dungeon_file); //cs:8 TODO: might want to use the real variable instead of unsigned char
    		  // if (map[i][j].hardness == 255) printf("%c", 'X');
    		  // else if (map[i][j].hardness > 255) printf("%c", 'O');
    		  // else printf("%c",' ');
    		  // temper++;
    		  // printf("%d ", map[i][j].hardness);
    		}
    		// printf("\n");
      }
      
      //quicly write corridors
      for (i = 0; i < nRows; i++){
      		for (j =0; j < nCols; j++){
      			map[i][j].value = 32;
      			if (map[i][j].hardness == 0) map[i][j].value = '#';
      		}
      	}
      
      //read rooms
      // int put = 0;
      Room room = {0,0,0,0};
      while((fread(&room.x, sizeof(uint8_t), 1, dungeon_file)) == 1){
        fread(&room.y, sizeof(uint8_t), 1, dungeon_file);
        fread(&room.width, sizeof(uint8_t), 1, dungeon_file);
        fread(&room.height, sizeof(uint8_t), 1, dungeon_file);
        write_room(map, room);
        // printf("%d\n", ++put);
        // if ((dungeon.num_rooms%20 == 0) && (dungeon.num_rooms != 0)){
        //   int ratio = dungeon.num_rooms/20 + 1;
        //   if (!realloc(dungeon.rooms, ratio*20)) return -1;
        // }
        addRoom(&dungeon, room);
      }
      
      //display corridor
      fclose(dungeon_file);

    }
  }else{
    //add rooms to dungeon
    while ((area < .2*(nRows*nCols) || count < 10) && (++tries < 2000)){
      Room room; // = {0, 0, 0, 0};
  		room.y = rand()%(nRows-2) + 1;
  		room.x = rand()%(nCols-2) + 1;
  		
  		if (!create_room(map, room.x, room.y, &room.width, &room.height)) continue; //continue loop if roo couldn't be created
  		area += room.width * room.height;
  		
      //get random point in each room
  		room_points[count].x = rand_gen(room.x, room.width + room.x -1);
  		room_points[count].y = rand_gen(room.y, room.height + room.y -1);
  		count++;
  		
  		//add room to rooms array TODO merge this with code above
  		addRoom(&dungeon, room);
	  }
    //connect random points
  	for (i =0; i < count-1; i++){
  		connect_rooms(map, room_points[i], room_points[i+1]);
  	}
  }

	//render world
	render(map);
	printf("%d tries %d\n", count, tries);
	
	//save
	//create directory if save is passed move to end of code
  if (save) {
    char* path = getenv("HOME");
    printf("path: %s\n", path);
    mkdir(strcat(path, "/.rlg327"),0766);
    strcat(path, "/dungeon");
    FILE* dungeon_file_l;
    if (!(dungeon_file_l = fopen(path, "w"))){
      fprintf(stderr, "Could not write map to file\n");
      return -1;
    }else{
      uint32_t temp = 0;//TODO remove all printfs
      printf("Started saving\n");
      //write dungeon title
      char* dungoen_title_l = "RLG327-S2017";
      fwrite(dungoen_title_l, 12, 1, dungeon_file_l); //cs: 12
      // strcpy(dungoen_title, temp_name); //TODO figure out dungeon title toendian
      printf("dungeon title: %s\n", dungoen_title_l);
      // free(temp_name);
      
      //write verison
      temp = 0;
      version = htobe32(temp);
      fwrite(&version, 4, 1, dungeon_file_l); //cs: 4
      printf("version: %d\n", be32toh(version));
      
      //write size of file
      temp = 16000; //change this to the math
      size_dungeon_file = htobe32(temp);
      fwrite(&size_dungeon_file, 4, 1, dungeon_file_l); //cs:4
      printf("size_dungeon_file: %d\n", be32toh(size_dungeon_file));
      
      //write hardness
      // int temper = 20;
      for (i = 0; i < nRows; i++){
    		for (j =0; j < nCols; j++){
    		  fwrite(&(map[i][j].hardness), sizeof(unsigned char), 1, dungeon_file_l); //cs:8 TODO: might want to use the real variable instead of unsigned char
    		}
      }
      
      //read rooms
      for (i = 0; i < dungeon.num_rooms; i++){
        Room room_l = dungeon.rooms[i];//TODO might want to staright up use dun.rooms[i] instead of temp variable
        fwrite(&room_l.x, sizeof(uint8_t), 1, dungeon_file_l);
        fwrite(&room_l.y, sizeof(uint8_t), 1, dungeon_file_l);
        fwrite(&room_l.width, sizeof(uint8_t), 1, dungeon_file_l);
        fwrite(&room_l.height, sizeof(uint8_t), 1, dungeon_file_l);
      }
      
      //display corridor
      fclose(dungeon_file_l);

    }
  }
  free(dungeon.rooms);
	return 0;
}

int create_room(Point map[][nCols], int x, int y, uint8_t* width, uint8_t* height){
	int i = 0, j= 0;
	//generate random length and width of room by genrating a random co-ord
	int max = nCols-2;
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
		  updatePoint(&map[j][i], 46);
		}
	}

	//return width and length of room respectively
	*width = end_x - x + 1;
  *height = end_y - y + 1;
	return 1;
}

int rand_gen(int min, int max){
	return (max >= min) ? (rand() % (max-min+1)) + min : -1;
}

int connect_rooms(Point map[][nCols], Point p, Point q){
	while (p.x != q.x){
		if (map[p.y][p.x].value != 46){
		  updatePoint(&map[p.y][p.x], 35);
		// 	map[p.y][p.x].value = 35;
		}
		p.x = (p.x > q.x) ? p.x-1 : p.x+1;
	}
	while (p.y != q.y){
		if (map[p.y][p.x].value != 46){
		  updatePoint(&map[p.y][p.x], 35);
		// 	map[p.y][p.x].value = 35;
		}
		p.y = (p.y > q.y) ? p.y-1 : p.y+1;
	}
	return 1;
}

int updatePoint(Point* p, char value){
  if (p->hardness == 255) return 1;
  p->value = value;
  return 0;
}

void render(Point map[][nCols]){
  int i = 0, j = 0;
  for (i = 0; i < nRows; i++){
		for (j =0; j < nCols; j++){
			printf("%c", map[i][j].value);
		}
		printf("\n");
	}
}

int write_room(Point map[][nCols], Room room){
  int i=0,j=0,m=0,n=0;
  n = room.x + room.width - 1;
  m = room.y + room.height - 1;
  for (i = room.y; i <= m; i++){
    for (j = room.x; j <= n; j++){
      updatePoint(&map[i][j], '.');
    }
  }
  return 0;
}

int addRoom(Dungeon* rlg, Room room){ //TODO debug with 20
  // if ((rlg->num_rooms%30 == 0) && (rlg->num_rooms != 0)){
  //   int ratio = rlg->num_rooms/30 + 1;
  //   if (!realloc(rlg->rooms, ratio*30)) return -1;
  // }
  rlg->rooms[rlg->num_rooms].x = room.x;
  rlg->rooms[rlg->num_rooms].y = room.y;
  rlg->rooms[rlg->num_rooms].width = room.width;
  rlg->rooms[rlg->num_rooms].height = room.height;
  rlg->num_rooms++;
  return 0;
}
