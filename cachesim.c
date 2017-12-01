#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//constant strings for comparing store command
static const char STORE[] = "store";
unsigned char memory[16777216];
int hitOrMiss = 0;

//use queue to keep track of least recently used, has blockTag and pointers to previous and next nodes
typedef struct BlockNode {
	
	struct BlockNode *prev, *next;
	int blockTag;

} BlockNode;

//each set has a SetRow, which is a doubly linked list to keep track of what blocks are in loaded in the set and which was most recently used
typedef struct SetRow {
	struct BlockNode *head, *tail;
	int numberWays;
	int framesFilled;
	int setNumber;
} SetRow;

//check if set is empty
int setEmpty(SetRow *setRow) {
	return (setRow->framesFilled == 0);
}

//check if set is full
int setFull(SetRow *setRow) {
	return (setRow->numberWays == setRow->framesFilled);
}

//remove the least recently used block aka the front block in the queue
void removeFront(SetRow *setRow) {
	if (setEmpty(setRow)) return;
	if (setRow->framesFilled == 1) {
		setRow->head = NULL;
		setRow->tail = NULL;
	}
	else {
		BlockNode *temp = setRow->head;
		setRow->head = setRow->head->next;
		setRow->head->prev = NULL;
		temp->next = NULL;
	}
	setRow->framesFilled--;
}



void retrieveFromMemory(int addTag, int address, unsigned int addValueSize,unsigned int* ret) {

	for (int i = 0; i < addValueSize; i++) {
		ret[i] = (unsigned int)memory[address + i];
	}
}


//adds a block to the end of the queue for the set
void addToSetRow(SetRow *setRow, int addTag, int address, int addValueSize, unsigned int *addValue) {
	
	BlockNode* temp = (BlockNode*)malloc(sizeof(BlockNode));
	temp->blockTag = addTag;
	if (setFull(setRow)) {
		removeFront(setRow);
		if (setRow->framesFilled == 0)
		{
			setRow->head = temp;
			setRow->tail = temp;
			setRow->tail->next = NULL;
			setRow->tail->prev = NULL;
		}
		else
		{
			temp->prev = setRow->tail;
			setRow->tail->next = temp;
			setRow->tail = temp;
			setRow->tail->next = NULL;
		}
	}
	else if (setEmpty(setRow)) {
		setRow->head = temp;
		setRow->tail = temp;
		setRow->tail->next = NULL;
		setRow->tail->prev = NULL;
		
	}
	else {
		temp->prev = setRow->tail;
		setRow->tail->next = temp;
		setRow->tail = temp;
		setRow->tail->next = NULL;
		
	}
	setRow->framesFilled++;
	
	for (int i = 0; i < addValueSize; i++) {
		memory[address + i] = addValue[i];
	}
}

//adds a block with address tag "addTag" to the end of the queue for specified set
void moveToEndSetRow(SetRow *setRow, int addTag) {
	BlockNode* temp = (BlockNode*)malloc(sizeof(BlockNode));
	temp->blockTag = addTag;
	if (setFull(setRow)) {
		removeFront(setRow);
		if (setRow->framesFilled == 0)
		{
			setRow->head = temp;
			setRow->tail = temp;
			setRow->tail->next = NULL;
			setRow->tail->prev = NULL;
			
		}
		else
		{
			temp->prev = setRow->tail;
			setRow->tail->next = temp;
			setRow->tail = temp;
			setRow->tail->next = NULL;
		}
		
	}
	else if (setEmpty(setRow)) {
		setRow->head = temp;
		setRow->tail = temp;
		setRow->tail->next = NULL;
		setRow->tail->prev = NULL;
	}
	else {
		temp->prev = setRow->tail;
		setRow->tail->next = temp;
		setRow->tail = temp;
		setRow->tail->next = NULL;
		
	}
	setRow->framesFilled++;
}

//stores addValue at addTag+addOffset in memory, if block with same tag as addTag exists in cache, overwrite in both cache and memory, otherwise just write in memory
void addressAccessedStore(SetRow *setRow, int addTag, int address, int addValueSize, unsigned int *addValue) {
	BlockNode *temp = setRow->head;
	hitOrMiss = 0;
	while (temp != NULL) {
		if (temp->blockTag == addTag) {
			if (setRow->framesFilled == 1) {
				hitOrMiss = 1;
				break;
			}
			if (temp == setRow->head) {
				// setRow->head = temp->next;
				removeFront(setRow);
				//setRow->framesFilled--;
				addToSetRow(setRow, addTag, address, addValueSize, addValue);   //moves existing block to end of the queue as it is most recently used
				hitOrMiss = 1;
				break;
			}
//			addToSetRow(setRow, addTag, address, addValueSize, addValue);   //moves existing block to end of the queue as it is most recently used
//			temp->prev->next = temp->next;
			hitOrMiss = 1;
			if(temp != setRow->tail)
			{
				temp->prev->next = temp->next;
				temp->next->prev = temp->prev;
				setRow->framesFilled--;
				moveToEndSetRow(setRow, addTag);
			}
			break;
		}
		temp = temp->next;
	}
	//store addValue in memory
	//unsigned int tempValue = addValue;													
	for (int i = 0; i < addValueSize; i++) {									
		memory[address + i] = addValue[i];
	}
}

//returns value at addTag+addOffset, checks if exists in cache, otherwise load from memory
void addressAccessedLoad(SetRow *setRow, int addTag, int address, int addValueSize,unsigned int* ret) {
	
	BlockNode *temp = setRow->head;
	
	while (temp != NULL) {
		if (temp->blockTag == addTag) {
			if (setRow->framesFilled == 1) { 
				hitOrMiss = 1;
				 retrieveFromMemory(addTag, address, addValueSize,ret);
				return;
			}
			if (temp == setRow->head) {
				// setRow->head = temp->next;
				removeFront(setRow);
				//setRow->framesFilled--;
				moveToEndSetRow(setRow, addTag);
				hitOrMiss = 1;
				 retrieveFromMemory(addTag, address, addValueSize,ret);
				return;
			}
			if (temp != setRow->tail)
			{
				temp->prev->next = temp->next;
				temp->next->prev = temp->prev;
//				(*(temp->prev)).next = temp->next;
//				(*(temp->next)).prev = temp->prev;
				setRow->framesFilled--;
				moveToEndSetRow(setRow, addTag);
			}
			hitOrMiss = 1;
			retrieveFromMemory(addTag, address, addValueSize,ret);
			return;
		}
		temp = temp->next;
	}
	hitOrMiss = 0;
	moveToEndSetRow(setRow, addTag);
	retrieveFromMemory(addTag, address, addValueSize,ret);
	
}



//returns log2 of input n
int log2(int n) {
	int r = 0;
	while (n >>= 1) r++;
	return r;
}

//returns a mod b
int modulo(int a, int b) {
	return a & (b - 1);
}

//returns bit string of m ones
int ones(int m) {
	return ((1 << m) - 1);
} 

int main(int argc, char* argv[]) {
	FILE* traceFile = fopen(argv[1], "r");
	int cacheSize = atoi(argv[2]) * 1024;
	int ways = atoi(argv[3]);
	int blockSize = atoi(argv[4]);
	int numFrames = cacheSize / blockSize;
	int sets = numFrames / ways;
	int offsetBits = log2(blockSize);
	int indexBits = log2(sets);
	int store = 0;
	char command[8];
	int address = 0;
	int accessSize = 0; 
	int addressTag = 0;
	int addressOffset = 0;
	int addressSet = 0;
	unsigned int* loadedValue = 0;
	//unsigned int value;
	unsigned int value[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

	SetRow cacheData[sets];
	SetRow* initialSet;

	//fill cache array with empty linkedlists of blocks at first
	for(int i = 0 ; i < sets ; i++) {
		initialSet = (SetRow*)malloc(sizeof(SetRow));
		initialSet->head = initialSet->tail = NULL;
		initialSet->numberWays = ways;
		initialSet->framesFilled = 0;
		initialSet->setNumber = i;
		cacheData[i] = *initialSet;
	}


	while (!feof(traceFile)) {
		strncpy(command, "\0", 1);
		fscanf(traceFile, "%s", command);
		if (strlen(command) == 0)
			break;
		if (strcmp(command, STORE) == 0) store = 1;
		else store = 0;
		fscanf(traceFile, "%x", &address);
		fscanf(traceFile, "%i", &accessSize);
		addressTag = address >> (offsetBits);
		addressOffset = address & ones(offsetBits);
		addressSet = ((address >> offsetBits) & ones(indexBits));
		
		if (store) {
			for (int i = 0; i < accessSize; i++)
			{
				fscanf(traceFile, "%2hhx", (unsigned char*)&value[i]);
			}
			
			addressAccessedStore(&cacheData[addressSet], addressTag, address, accessSize, value);
			printf("%s 0x%x ", command, address);
			if (hitOrMiss) printf("hit\n");
			else printf("miss\n");
			//printf("Address Tag: %x; Address Offset: %x; Address Set: %x\n", addressTag, addressOffset, addressSet);
		}

		else {
			unsigned int loadedValue[accessSize];
			addressAccessedLoad(&cacheData[addressSet], addressTag, address, accessSize, loadedValue);
			printf("%s 0x%x ", command, address);
			if (hitOrMiss) printf("hit ");
			else printf("miss ");
			for (int i = 0; i < accessSize; i++)
			{
				printf("%0*x", 2, loadedValue[i]);
			}
			printf("\n");
			
			//printf("Address Tag: %x; Address Offset: %x; Address Set: %x\n", addressTag, addressOffset, addressSet);
		}

		// printf("Command: %s\n", command);
		// printf("Address: %x\n", address);
		// printf("Access Size %i\n", accessSize);
		// printf("Value: %x\n", value);
	}

	return EXIT_SUCCESS;

}