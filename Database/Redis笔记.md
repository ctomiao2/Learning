\_\_attribute\_\_((packed))：GCC特有语法，告诉编译器使用紧凑模式。紧凑\非紧凑模式解释如下：

    在GCC下：struct my{ char ch; int a;} sizeof(int)=4;sizeof(my)=8;（非紧凑模式）
    在GCC下：struct my{ char ch; int a;}__attrubte__ ((packed)) sizeof(int)=4;sizeof(my)=5

\##：把两个宏参数贴合在一起, eg: 

    #define SDS_HDR(T,s) ((struct sdshdr##T *)((s)-(sizeof(struct sdshdr##T))))
	
	SDS_HDR(16, "abc")等价于((struct sdshdr16 *)(("abc")-(sizeof(struct sdshdr16))))

**Simple Dynamic String (SDS)**：Redis字符串数据结构，数据结构如下：

    struct __attribute__ ((__packed__)) sdshdr8 {
		uint8_t len;
		uint8_t alloc;
		unsigned char flags;
		char buf[];
	}

还有sdshdr16、sdshdr32、sdshdr64结构相同不一一列出。构造函数：

    typedef char *sds;
	sds sdsnewlen(const void *init, size_t initlen);

实现原理是分配一个hdrlen+initlen+1大小的连续内存,其中hdrlen等于根据initlen决定采用何种类型sdshdr的size，该内存的前hdrlen被用来初始化sdshdr，后initlen+1用来拷贝init字符串，函数返回hdrlen开始的字符串，即init字符串的拷贝内容串。访问该字符串的长度时无需使用strlen，而是通过将指针往前偏移hdrlen还原得到sdshdr实例，直接访问sdshdr.len。更详细源码参见sds.h\sds.c。

**list**：Redis自己实现了双向链表，数据结构如下：
	
	// 链表节点 
	typedef struct listNode {
		struct listNode *prev;
		struct listNode *next;
		void *value;
	} listNode;
	
	// 双端链表
	typedef struct list {
		listNode *head;
		listNode *tail;
		void* (*dup)(void *ptr); //自定义拷贝value函数
		void (*free)(void *ptr); //自定义销毁value函数
		int (*match)(void *ptr, void *key); //自定义比较value函数
		unsigned long len;	
	} list;
	
	//迭代器
	typedef struct listIter {
	    listNode *next;
	    int direction;
	} listIter;

**dict**：

反转二进制位算法，(abcdefgh)<sub>2</sub> ==> (hgfedcba)<sub>2</sub>

	1. 对调相邻的1位(abcdefgh-> badcfehg):
	   v = (abcdefgh)
	   (badcfehg) = (0a0c0e0g)|(b0d0f0h0) = ((v>>1)&(01010101)) | ((v&01010101)<<1)
	   即 v = ((v >> 1) & 0x55555555) | ((v & 0x55555555)<< 1);
    2. 对调相邻的2位(abcdefgh-> cdabghef):
       同上可得：v = ((v >> 4) & 0x0F0F0F0F) | ((v & 0x0F0F0F0F) << 4);
	3. 对调相邻的4位(abcd efgh-> efgh abcd)：
	   同上可得：v = ((v >> 4) & 0x0F0F0F0F) | ((v & 0x0F0F0F0F) << 4);
	4. 对调相邻的8位(相邻的字节)：
	   同上可得：v = ((v >> 8) & 0x00FF00FF) | ((v & 0x00FF00FF) << 8);
    5. 对调相邻的16位（相邻的两字节）
       同上可得：v = (v >> 16) | (v<< 16);
	
	至此32位数的二进制位被翻转过来，上面几个步骤也可以倒过来，最后结果是一样。因此redis里翻转二进制位的算法是这样：
	
	static unsigned long rev(unsigned long v) {
	    unsigned long s = 8 * sizeof(v); // bit size; must be power of 2
	    unsigned long mask = ~0;
	    while ((s >>= 1) > 0) {
	        mask ^= (mask << s);
	        v = ((v >> s) & mask) | ((v << s) & ~mask);
	    }
	    return v;
	}


**quicklist：**

quicklistNode采用位域声明方式：

	typedef struct quicklistNode {
	    struct quicklistNode *prev;
	    struct quicklistNode *next;
	    unsigned char *zl;
	    unsigned int sz;             /* ziplist size in bytes */
	    unsigned int count : 16;     /* count of items in ziplist */
	    unsigned int encoding : 2;   /* RAW==1 or LZF==2 */
	    unsigned int container : 2;  /* NONE==1 or ZIPLIST==2 */
	    unsigned int recompress : 1; /* was this node previous compressed? */
	    unsigned int attempted_compress : 1; /* node can't compress; too small */
	    unsigned int extra : 10; /* more bits to steal for future usage */
	} quicklistNode;
	
	这里从变量count开始，都采用了位域的方式进行数据的内存声明，使得6个unsigned int变量只用到了一个unsigned int的内存大小。
	C语言支持位域的方式对结构体中的数据进行声明，也就是可以指定一个类型占用几位：
	1) 如果相邻位域字段的类型相同，且其位宽之和小于类型的sizeof大小，则后面的字段将紧邻前一个字段存储，直到不能容纳为止；
	2) 如果相邻位域字段的类型相同，但其位宽之和大于类型的sizeof大小，则后面的字段将从新的存储单元开始，其偏移量为其类型大小的整数倍；
	3) 如果相邻的位域字段的类型不同，则各编译器的具体实现有差异，VC6采取不压缩方式，Dev-C++采取压缩方式；
	4) 如果位域字段之间穿插着非位域字段，则不进行压缩；
	5) 整个结构体的总大小为最宽基本类型成员大小的整数倍。

**\_\_builtin_expect：** GCC编译器特有的语法，用于分支预测。
声明形式：long __builtin_expect(long exp, long c);
期望 exp 表达式的值等于常量 c, 如果 c 的值为0(即期望的函数返回值), 那么执行if分支的的可能性小, 否则执行 else 分支的可能性小(函数的返回值等于第一个参数 exp)，GCC在编译过程中，会将可能性更大的代码紧跟着前面的代码，从而减少指令跳转带来的性能上的下降, 达到优化程序的目的。 eg:

	#define likely(x)      __builtin_expect(!!(x), 1)
	#define unlikely(x)    __builtin_expect(!!(x), 0)
	
	// 期望 x == 0, 所以执行func()的可能性小
	if (unlikely(x))
	{
	    func();
	}
	else
	{
	　　//do someting
	}

	// 期望 x == 1, 所以执行func()的可能性大
	if (likely(x))
	{
	    func();
	}
	else
	{
	　　//do someting
	}

原理：if else 句型编译后, 一个分支的汇编代码紧随前面的代码,而另一个分支的汇编代码需要使用JMP指令才能访问到。通过JMP访问需要更多的时间, 在复杂的程序中,有很多的if else句型,又或者是一个有if else句型的库函数,每秒钟被调用几万次, 通常程序员在分支预测方面做得很糟糕, 编译器又不能精准的预测每一个分支,这时JMP产生的时间浪费就会很大。

**ziplist：** https://redisbook.readthedocs.io/en/latest/compress-datastruct/ziplist.html


