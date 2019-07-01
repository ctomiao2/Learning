\_\_attribute\_\_((packed))：GCC特有语法，告诉编译器使用紧凑模式。紧凑\非紧凑模式解释如下：

    在GCC下：struct my{ char ch; int a;} sizeof(int)=4;sizeof(my)=8;（非紧凑模式）
    在GCC下：struct my{ char ch; int a;}__attrubte__ ((packed)) sizeof(int)=4;sizeof(my)=5

\##：把两个宏参数贴合在一起, eg: 

    #define SDS_HDR(T,s) ((struct sdshdr##T *)((s)-(sizeof(struct sdshdr##T))))
	
	SDS_HDR(16, "abc")等价于((struct sdshdr16 *)(("abc")-(sizeof(struct sdshdr16))))

Simple Dynamic String (SDS)：Redis字符串数据结构，数据结构如下：

    struct __attribute__ ((__packed__)) sdshdr8 {
		uint8_t len;
		uint8_t alloc;
		unsigned char flags;
		char buf[];
	}

还有sdshdr16、sdshdr32、sdshdr64结构相同不一一列出。构造函数：

    typedef char *sds;
	sds sdsnewlen(const void *init, size_t initlen);

实现原理是分配一个hdrlen+initlen+1大小的连续内存,其中hdrlen等于根据initlen决定采用何种类型sdshdr的size，该内存的前hdrlen被用来初始化sdshdr，后initlen+1用来拷贝init字符串，函数返回hdrlen开始的字符串，即init字符串的拷贝内容串。访问该字符串的长度时无需使用strlen，而是通过将指针往前偏移hdrlen还原得到sdshdr实例，直接访问sdshdr.len。
