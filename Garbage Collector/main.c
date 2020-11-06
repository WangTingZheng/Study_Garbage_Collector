#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include "main.h"

typedef enum {
	OBJ_INIT,
	OBJ_PAIR
} ObjectType;

/*
	����һ���ڴ�飬�����洢����
	������������ݽṹ����֯�ڴ�飬���������ʱ���ѯ
*/
typedef struct sObject {
	// ����ṹ��ָ����һ���ڴ��
	struct sObject* next;
	// ����Ƿ�ʹ��
	unsigned char marked;  
	// ��ʶ��INT���ͻ���PAIR
	ObjectType type;
	// ��һ��������洢�ڴ�ռ��ֵ
	union {
		// INT�����ֵ
		int value;
		//PAIR�����ֵ
		struct {
			//�洢һ�Զ���
			struct sObject* head;
			struct sObject* tail;
		};
	};
}Object;

// С�����
//�����ջ�ڴ���С
#define  STACK_MAX 256
#define  INITIAL_GC_THRESHOLD 128

typedef struct {
	// ͳ������
	// ��¼�����ջ�д洢�Ķ���ĸ���
	int numObjects;
	// ��¼�����ջ���ܴ洢��������ĸ���
	int maxObjects;
	// ����ṹ
	// �洢�����ջ��������ͷ�ڵ�
	Object* firstObject;
	// �洢�½������ջ
	Object* stack[STACK_MAX];
	// �����ջ��ջ��ָ��
	int stackSize;
}VM;

void gc(VM* vm);

// ��ʼ�������
VM* newVM() {
    // ����������ռ�
	VM* vm = calloc(1,sizeof(VM));
	// ��ʼ�������ջ��ָ��Ϊ0
	vm->stackSize = 0;
	// ��ʼ��������洢�Ķ�������Ϊ0
	vm->numObjects = 0;
	// ��ʼ��������洢������������Ϊ�趨ֵ
	vm->maxObjects = INITIAL_GC_THRESHOLD;
	return vm;
}

// �ͷ������
void  freeVM(VM *vm) {
	// ���������ջ��ָ��Ϊ0
	vm->stackSize = 0;
	gc(vm);
	free(vm);
}

// �����������ջ
// ��ĳһ�������ֵ���뵽�����ջ�ռ���
void push(VM* vm,Object* value){
	// ��������ջ��ָ���������棬˵���Ѿ�����
	// �Ͳ����ڼ�������ˣ��ͱ���Stack overflow��
	assert(vm->stackSize < STACK_MAX,"Stack overflow!");
	// �������ջ��ָ��������һλ��Ȼ�����ֵ
	vm->stack[vm->stackSize++] = value;
}

// �������ջ�ռ䵯��һ������
Object* pop(VM* vm) {
	// ��������ջ��ָ���������棬˵��ջ��û�ж�����
	// �Ͳ������ⵯ�����ˣ��ͱ���Stack underflow��
	assert(vm->stackSize > 0,"Stack underflow!");
	// �����ջ��ջ������һ�����󣬲���ջ��ָ��������
	// Ȼ�󷵻ص����Ķ���
	return vm->stack[--vm->stackSize];
}

// �½�����
Object* newObject(VM* vm,ObjectType type) {
	// �ж��Ƿ���Ҫ�ͷ�
	// ���������ڴ洢�Ķ��������������ɴ洢������˵��ջ�ռ��Ѿ�����
	// �˿�����GC�����ջ�ռ��������
	if (vm->numObjects == vm->maxObjects)
		gc(vm);
	// ����֮�⣬˵��ջ�ռ仹û��
	// ����������֮����Ϊ��GC�����Կ���
	{
		// ��������ڴ�ռ�
		Object* object = (Object*)calloc(1,sizeof(Object));
		// ���ö�������
		object->type = type;
		// ���ö���δ����ǣ���Ϊ�ǿռ��ǿյģ�δ��ʹ��
		object->marked = 0;

		// �Ѷ�����뵽ջ�����ͷ��
		object->next = vm->firstObject;
		vm->firstObject = object;

		// ջ�ռ����������һ
		vm->numObjects ++;

		// �����´����Ķ���
		return object;
	}
}

// ��д������ÿ�����͵Ķ���ѹ���������ջ��
// ��INT���͵�ֵ���뵽�����ջ��
void pushInt(VM* vm,int intValue){
	// �½�һ��INT���͵Ķ���
	Object* object = newObject(vm,OBJ_INIT);
	// ��ʼ�������ֵ
	object->value = intValue;

	// �����������뵽�����ջ��
	push(vm,object);
}


//TODO: ��֪�������Pair��������ô����
// ��PAIR���͵�ֵ���뵽�����ջ��
Object* pushPair(VM* vm) {
	// �½�һ���յ�PAIR����
	Object* object = newObject(vm,OBJ_PAIR);
	// 
	object->tail = pop(vm);
	object->head = pop(vm);

	push(vm,object);
	return object;
}

// ���,��Ǻ��ʾ�ö���ɾ���ˣ����ڴ沢û����ȫɾ��
void mark(Object* object) {
	// ��������Ƕ��
	// ��������ѱ���ǣ��Ͳ��ñ����
	if (object->marked)
		return;

	// ��Ƕ���˵������ռ��Ǳ�ʹ�õ�
	object->marked = 1;
	// ��������������Pair
	if (object->type == OBJ_PAIR)
	{
		// �ݹ���Pair������Ӷ���
		mark(object->head);
		mark(object->tail);
	}
}

// �������������еĶ���
void markAll(VM* vm){
	int i=0;
	// ����ջ�����д洢�Ķ���
	// ���������洢�Ķ���
	for(;i<vm->stackSize;i++)
		mark(vm->stack[i]);
}

// �����ǵ��ڴ�
void sweep(VM* vm)
{
	// �½������ջ��ͷ�ڵ�Ķ���
	Object** object = &vm->firstObject;
	// ������󻹴���
	while(*object) {
		// �������δ����ǣ�˵��û�б�ʹ��
		// û�б�ʹ���ˣ�����Ҫ������
		if (!(*object)->marked) {
			// ��ʱ����Ҫ�������ĵ�ַ������һ������
			Object* unreached = *object;

			// �ѵ�ǰ��������ΪҪ��������������������е���һ������
			*object = unreached->next;
			// �ͷ�Ҫ����Ķ���
			free(unreached);

			// ������洢����ռ��������һ
			vm->numObjects--;
		}
		// ��������Ѿ�����ǣ�˵������ʹ�ã���������
		else {
			//TODO: ����ʹ�õĶ���ռ�ֻ��һ�����ڵ�����������
			// �����ǣ���δ�����룬������һ��GC�б�����
			(*object)->marked = 0;
			// �ƶ���ǰ������һ�������Ա�����һ��ѭ���м�������
			object = &(*object)->next;
		}
	} 
}

// ���գ�����̬����
void gc(VM* vm) {
	// ��ȡ������洢�Ķ�������
	int numObjects = vm->numObjects;

	// ���������洢�����ж���
	markAll(vm);
	// ��������δ��ʹ�õĶ���
	sweep(vm);

	// ÿ���ռ�֮�����Ǹ���maxObjects�����Խ����ռ������ڻ�Ķ���Ϊ��׼��
	// �˷��������ǵĶ����Ż�еĶ������������Ӷ����ӡ�
	// ͬ����Ҳ������һЩ�������ձ��ͷŵ����Զ����١�
	// �����������ֵԽ��GC��Խ�������ͬ������ڴ��GC����Խ�٣�Խ��ʡʱ�䣬��ΪGCҲ����Ҫʱ���
	// �����������ֵԽС��Խ��ʡ�ڴ�ռ�
	// �Դﵽ��֤�ܴ��¶����ǰ���£���ʡ�ռ�
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