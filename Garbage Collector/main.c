
#include <stdlib.h>
#include <memory.h>
#include <assert.h>

typedef enum {
	OBJ_INIT,
	OBJ_PAIR
} ObjectType;


typedef struct sObject {
	// ����ṹ
	struct sObject* next;
	// ���
	unsigned char marked;  

	ObjectType type;

	union {
		/* OBJ_INT */
		int value;

		/* OBJ_PAIR */
		struct {
			struct sObject* head;
			struct sObject* tail;
		};
	};
}Object;

// С�����
#define  STACK_MAX 256
#define  INITIAL_GC_THRESHOLD 128

typedef struct {
	// ͳ������
	int numObjects;
	int maxObjects;
	// ����ṹ
	Object* firstObject;
	Object* stack[STACK_MAX];
	int stackSize;
}VM;

void gc(VM* vm);

// ��ʼ�������

VM* newVM() {
	VM* vm = calloc(1,sizeof(VM));
	vm->stackSize = 0;

	vm->numObjects = 0;
	vm->maxObjects = INITIAL_GC_THRESHOLD;
	return vm;
}

// �ͷ������
void  freeVM(VM *vm) {
	vm->stackSize = 0;
	gc(vm);
	free(vm);
}

// �����������ջ

void push(VM* vm,Object* value){
	assert(vm->stackSize < STACK_MAX,"Stack overflow!");
	vm->stack[vm->stackSize++] = value;
}

Object* pop(VM* vm) {
	assert(vm->stackSize > 0,"Stack underflow!");
	return vm->stack[--vm->stackSize];
}

// �½�����
Object* newObject(VM* vm,ObjectType type) {
	// �ж��Ƿ���Ҫ�ͷ�
	if (vm->numObjects == vm->maxObjects)
		gc(vm);
	{
		Object* object = (Object*)calloc(1,sizeof(Object));
		object->type = type;
		object->marked = 0;

		object->next = vm->firstObject;
		vm->firstObject = object;

		vm->numObjects ++;

		return object;
	}
}

// ��д������ÿ�����͵Ķ���ѹ���������ջ��
void pushInt(VM* vm,int intValue){
	Object* object = newObject(vm,OBJ_INIT);
	object->value = intValue;

	push(vm,object);
}

Object* pushPair(VM* vm) {
	Object* object = newObject(vm,OBJ_PAIR);
	object->tail = pop(vm);
	object->head = pop(vm);

	push(vm,object);
	return object;
}

// ���,��Ǻ��ʾ�ö���ɾ���ˣ����ڴ沢û����ȫɾ��
void mark(Object* object) {
	// ��������Ƕ��
	if (object->marked)
		return;

	object->marked = 1;
	if (object->type == OBJ_PAIR)
	{
		mark(object->head);
		mark(object->tail);
	}
}

void markAll(VM* vm){
	int i=0;
	for(;i<vm->stackSize;i++)
		mark(vm->stack[i]);
}

// �����ǵ��ڴ�
void sweep(VM* vm)
{
	Object** object = &vm->firstObject;
	while(*object) {
		if (!(*object)->marked) {
			// �Ƴ�
			Object* unreached = *object;

			*object = unreached->next;
			free(unreached);

			vm->numObjects--;
		}
		else {
			(*object)->marked = 0;
			object = &(*object)->next;
		}
	} 
}

// ���գ�����̬����
void gc(VM* vm) {
	int numObjects = vm->numObjects;

	markAll(vm);
	sweep(vm);

	vm->maxObjects = vm->numObjects * 2;
}

void test1() {
	printf("Test 1: Objects on stack are preserved.\n");
	{
		VM* vm = newVM();
		pushInt(vm, 1);
		pushInt(vm, 2);

		gc(vm);
		assert(vm->numObjects == 2, "Should have preserved objects.");
		freeVM(vm);
	}
}

void test2() {
	printf("Test 2: Unreached objects are collected.\n");
	{
		VM* vm = newVM();
		pushInt(vm, 1);
		pushInt(vm, 2);
		pop(vm);
		pop(vm);

		gc(vm);
		assert(vm->numObjects == 0, "Should have collected objects.");
		freeVM(vm);
	}
}

void test3() {
	printf("Test 3: Reach nested objects.\n");
	{
		VM* vm = newVM();
		pushInt(vm, 1);
		pushInt(vm, 2);
		pushPair(vm);
		pushInt(vm, 3);
		pushInt(vm, 4);
		pushPair(vm);
		pushPair(vm);

		gc(vm);
		assert(vm->numObjects == 7, "Should have reached objects.");
		freeVM(vm);
	}
}

void test4() {
	printf("Test 4: Handle cycles.\n");
	{
		VM* vm = newVM();
		pushInt(vm, 1);
		pushInt(vm, 2);

		{
			Object* a = pushPair(vm);
			pushInt(vm, 3);
			pushInt(vm, 4);

			{
				Object* b = pushPair(vm);
				/* Set up a cycle, and also make 2 and 4 unreachable and collectible. */
				a->tail = b;
				b->tail = a;
			}
		}

		gc(vm);
		assert(vm->numObjects == 4, "Should have collected objects.");
		freeVM(vm);
	}
}

void perfTest() {
	printf("Performance Test.\n");
	{
		VM* vm = newVM();
		int i = 0;
		int j = 0;
		int k = 0;
		for (; i < 1000; i++) {
			for (; j < 20; j++) {
				pushInt(vm, i);
			}

			for (; k < 20; k++) {
				pop(vm);
			}
		}
		freeVM(vm);
	}
}

int main(int argc, const char * argv[]) {
	test1();
	test2();
	test3();
	test4();
	perfTest();

	return 0;
}