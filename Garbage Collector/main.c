#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include "main.h"

typedef enum {
	OBJ_INIT,
	OBJ_PAIR
} ObjectType;

/*
	定义一个内存块，用来存储对象
	采用链表的数据结构来组织内存块，方便清理的时候查询
*/
typedef struct sObject {
	// 链表结构，指向下一个内存块
	struct sObject* next;
	// 标记是否被使用
	unsigned char marked;  
	// 标识是INT类型还是PAIR
	ObjectType type;
	// 用一个联合体存储内存空间的值
	union {
		// INT对象的值
		int value;
		//PAIR对象的值
		struct {
			//存储一对对象
			struct sObject* head;
			struct sObject* tail;
		};
	};
}Object;

// 小虚拟机
//虚拟机栈内存块大小
#define  STACK_MAX 256
#define  INITIAL_GC_THRESHOLD 128

typedef struct {
	// 统计数量
	// 记录虚拟机栈中存储的对象的个数
	int numObjects;
	// 记录虚拟机栈中能存储的最多对象的个数
	int maxObjects;
	// 链表结构
	// 存储虚拟机栈（链表）的头节点
	Object* firstObject;
	// 存储新建对象的栈
	Object* stack[STACK_MAX];
	// 虚拟机栈的栈顶指针
	int stackSize;
}VM;

void gc(VM* vm);

// 初始化虚拟机
VM* newVM() {
    // 申请虚拟机空间
	VM* vm = calloc(1,sizeof(VM));
	// 初始化虚拟机栈顶指针为0
	vm->stackSize = 0;
	// 初始化虚拟机存储的对象数量为0
	vm->numObjects = 0;
	// 初始化虚拟机存储的最多对象数量为设定值
	vm->maxObjects = INITIAL_GC_THRESHOLD;
	return vm;
}

// 释放虚拟机
void  freeVM(VM *vm) {
	// 重置虚拟机栈顶指针为0
	vm->stackSize = 0;
	gc(vm);
	free(vm);
}

// 操作虚拟机堆栈
// 把某一个对象的值加入到虚拟机栈空间中
void push(VM* vm,Object* value){
	// 如果虚拟机栈顶指针在最上面，说明已经满了
	// 就不能在加入对象了，就报错“Stack overflow”
	assert(vm->stackSize < STACK_MAX,"Stack overflow!");
	// 虚拟机的栈顶指针向上升一位，然后赋入初值
	vm->stack[vm->stackSize++] = value;
}

// 从虚拟机栈空间弹出一个对象
Object* pop(VM* vm) {
	// 如果虚拟机栈顶指针在最下面，说明栈中没有对象了
	// 就不能往外弹对象了，就报错“Stack underflow”
	assert(vm->stackSize > 0,"Stack underflow!");
	// 虚拟机栈从栈顶弹出一个对象，并把栈顶指针向下移
	// 然后返回弹出的对象
	return vm->stack[--vm->stackSize];
}

// 新建对象
Object* newObject(VM* vm,ObjectType type) {
	// 判断是否需要释放
	// 如果虚拟机内存储的对象数量等于最大可存储数量，说明栈空间已经满了
	// 此刻启动GC程序对栈空间进行清理
	if (vm->numObjects == vm->maxObjects)
		gc(vm);
	// 除此之外，说明栈空间还没满
	// 或者是满了之后因为被GC了所以空了
	{
		// 申请对象内存空间
		Object* object = (Object*)calloc(1,sizeof(Object));
		// 设置对象类型
		object->type = type;
		// 设置对象未被标记，因为是空间是空的，未被使用
		object->marked = 0;

		// 把对象加入到栈链表的头部
		object->next = vm->firstObject;
		vm->firstObject = object;

		// 栈空间对象数量加一
		vm->numObjects ++;

		// 返回新创建的对象
		return object;
	}
}

// 编写方法将每种类型的对象压到虚拟机的栈上
// 将INT类型的值加入到虚拟机栈中
void pushInt(VM* vm,int intValue){
	// 新建一个INT类型的对象
	Object* object = newObject(vm,OBJ_INIT);
	// 初始化对象的值
	object->value = intValue;

	// 将这个对象加入到虚拟机栈中
	push(vm,object);
}


//TODO: 不知道这里的Pair类型是怎么回事
// 将PAIR类型的值加入到虚拟机栈中
Object* pushPair(VM* vm) {
	// 新建一个空的PAIR对象
	Object* object = newObject(vm,OBJ_PAIR);
	// 
	object->tail = pop(vm);
	object->head = pop(vm);

	push(vm,object);
	return object;
}

// 标记,标记后表示该对象被删除了，当内存并没有完全删除
void mark(Object* object) {
	// 避免陷入嵌套
	// 如果对象已被标记，就不用标记了
	if (object->marked)
		return;

	// 标记对象，说明对象空间是被使用的
	object->marked = 1;
	// 如果对象的类型是Pair
	if (object->type == OBJ_PAIR)
	{
		// 递归标记Pair对象的子对象
		mark(object->head);
		mark(object->tail);
	}
}

// 标记虚拟机内所有的对象
void markAll(VM* vm){
	int i=0;
	// 遍历栈中所有存储的对象
	// 标记虚拟机存储的对象
	for(;i<vm->stackSize;i++)
		mark(vm->stack[i]);
}

// 清理标记的内存
void sweep(VM* vm)
{
	// 新建虚拟机栈的头节点的对象
	Object** object = &vm->firstObject;
	// 如果对象还存在
	while(*object) {
		// 如果对象未被标记，说明没有被使用
		// 没有被使用了，就需要被清理
		if (!(*object)->marked) {
			// 暂时保存要清除对象的地址到另外一个变量
			Object* unreached = *object;

			// 把当前对象设置为要清理对象的在虚拟机链表中的下一个对象
			*object = unreached->next;
			// 释放要清理的对象
			free(unreached);

			// 虚拟机存储对象空间的数量减一
			vm->numObjects--;
		}
		// 如果对象已经被标记，说明正在使用，不能清理
		else {
			//TODO: 正在使用的对象空间只有一个周期的生命？？？
			// 清除标记，如未被申请，会在下一个GC中被清理
			(*object)->marked = 0;
			// 移动当前对象到下一个对象，以便在下一个循环中继续处理
			object = &(*object)->next;
		}
	} 
}

// 回收，并动态增加
void gc(VM* vm) {
	// 获取虚拟机存储的对象数量
	int numObjects = vm->numObjects;

	// 标记虚拟机存储的所有对象
	markAll(vm);
	// 清理所有未被使用的对象
	sweep(vm);

	// 每次收集之后，我们更新maxObjects——以进行收集后仍在活动的对象为基准。
	// 乘法器让我们的堆随着活动中的对象数量的增加而增加。
	// 同样，也会随着一些对象最终被释放掉而自动减少。
	// 对象数量最大值越大，GC得越慢，清除同样多的内存的GC次数越少，越节省时间，因为GC也是需要时间的
	// 对象数量最大值越小，越节省内存空间
	// 以达到保证能存下对象的前提下，节省空间
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