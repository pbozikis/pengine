#define ENTITY_CAP 10
int EntityNum = 0;

typedef struct transform
{
	int entity_id;
	int x, y;
	int orx, ory;
	int width, height;
	char direction;
}Transform;

typedef struct pixels
{
	int entity_id;
	int size;
	int width, height;
	void * memory;
}Pixels;