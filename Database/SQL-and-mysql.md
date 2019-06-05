**事务(Transaction)：** 数据库的一系列操作，要么全部完成要么全部撤销，不允许只完成一部分。

特性：**ACID**

- **原子性(Atomicity)：** 数据库的基本操作单位，要么都执行，要么都不执行
- **一致性(Consistency)：** 事务执行的结果是从一个一致性状态转换到另一个一致性状态，当数据库只包含成功事务的结果时就是一致性状态，而当操作到一半时因为故障停止剩下的事务操作导致只写入一部分的结果就是不一致性，mysql innodb在这种情况发生时，会在重启时通过
日志回滚到正确的状态。
- **隔离性(Isolation)：** 一个事务的执行不应被其他事务干扰
- **持久性(Durability)**：事务一旦提交，对数据库的改变应该是永久的，接下来的其他任何操作或故障都不应对其执行结果产生影响

**事务隔离级别：**

- 未提交读(Read Uncommited)：允许脏读，可能读到其他session未提交事务的修改。
- 已提交读(Read Commited)：大部分sql数据库的默认隔离级别，但mysql innodb不是。可避免脏读，但不可重复读。
- 可重复读(Repeatable Read)：mysql innodb的默认隔离级别，可重复读但存在幻读问题。
- 串行化(Serializable)：每次读都会获得表级共享锁（S锁），可避免脏读幻读不可重复读的问题，但读写会相互阻塞，一般不会用这个隔离级别。

几个概念：

- 脏读： 一个事务对数据进行了修改，但还未提交，另一个事务访问并使用了未提交的数据。若第一个事务在接下来回滚了修改，第二个事务访问的则是脏数据。
- 可重复读：同一个事务两次相同的查询得到的结果保证相同则是可重复读，反之则不可重复读。例如在可提交读隔离级别下，在第一个事务的两次读期间，第二个事务修改了读到的数据，那么第一个事务第二次读到的跟第一次读到的结果就是不一样的，尽管第一个事务两次操作期间没有发生任何修改并且两次读的语句一模一样。
- 幻读：同一个事务在两次读和写操作期间，另一个事务新增了或修改了读的数据，则第一个事务的写操作可能会发生失败，原因是写的时候发现数据库的数据已变更，写的操作是基于前一次读到的数据时是不靠谱的。eg:
	
	
		Session1                                                   Session2
		
		mysql> start transaction
		Query OK, 0 rows affected (0 sec) 
		mysql> select * from repeatable_table
		Empty set (0 sec)
																   mysql> start transaction;
																   Query OK, 0 rows affected (0 sec)
																   mysql> insert into repeatable_table values(1, 'first_row')
																   Query OK, 1 rows affected (0 sec)
																   mysql> insert into repeatable_table values(2, 'second_row')
																   Query OK, 1 rows affected (0 sec)
																   mysql> commit;
																   Query OK, 0 rows affected (0 sec)
																   mysql> select * from repeatable_table
																   +--------------+-------------+
																   |      id      |     text    |
																   +--------------+-------------+
																   |      1       |  first_row  |
															       +--------------+-------------+
																   |      2       |  second_row |
															       +--------------+-------------+																									   2 rows in set (0 sec)
		mysql> select * from repeatable_table
		Empty set (0 sec)
		mysql> update repeatable_table set text='modified' where id=1
		Query OK, 1 rows affected (0 sec)
		mysql> select * from repeatable_table
		+--------------+-------------+
		|      id      |     text    |
		+--------------+-------------+
		|      1       |   modified  |
        +--------------+-------------+
		1 row in set (0 sec)
		mysql> commit
		Query OK, 0 rows affected(0 sec)
		
		mysql> select * from repeatable_table
	    +--------------+-------------+
	    |      id      |     text    |
	    +--------------+-------------+
	    |      1       |  modified   |
        +--------------+-------------+
	    |      2       |  second_row |
        +--------------+-------------+																						
        2 rows in set (0 sec)
		
		mysql> create table repeatable_table_copy(id integer, text varchar(200))
		Query OK, 0 rows affected (0 sec)
		mysql> start transaction
		Query OK, 0 rows affected (0 sec)
	    mysql> select * from repeatable_table
	    +--------------+-------------+
	    |      id      |     text    |
	    +--------------+-------------+
	    |      1       |  modified   |
        +--------------+-------------+
	    |      2       |  second_row |
        +--------------+-------------+																						
        2 rows in set (0 sec)
																  mysql> start transaction
																  Query OK, 0 rows affected (0 sec)
																  mysql> update repeatable_table set text='first_row' where id=1
																  Query OK, 1 rows affected (0 sec)
																  mysql> commit;
																  Query OK, 0 rows affected (0 sec)
		
	    mysql> insert into repeatabl_table_copy (select * from repeatable_table)
	    Query OK, 2 rows affected (0 sec)
		Records: 2 Dumplicates: 0 Warnings: 0
	    mysql> select * from repeatable_table
	    +--------------+-------------+
	    |      id      |     text    |
	    +--------------+-------------+
	    |      1       |  modified   |
        +--------------+-------------+
	    |      2       |  second_row |
        +--------------+-------------+																						
        2 rows in set (0 sec)
		
		mysql> select * from repeatable_table_copy
	    +--------------+-------------+
	    |      id      |     text    |
	    +--------------+-------------+
	    |      1       |  first_row  |
        +--------------+-------------+
	    |      2       |  second_row |
        +--------------+-------------+																						
        2 rows in set (0 sec)
		
												
结论： 幻读不幻写，在Reaptable Read隔离级别下的事务，读数据是从快照读从而保证可重复读，但写数据时是直接更新物理数据库，从而不存在幻写的问题。 
如果使用当前读（即加锁读），就会读到“最新的”“提交”读的结果。 eg:

	select * from repeatable_table lock in share mode  // 加共享锁(S锁)
	select * from repeatable_table for update  //加排他锁(X锁)

四种隔离级别对比：

	=================================================================================================================
       隔离级别                       脏读（Dirty Read）          不可重复读（NonRepeatable Read）     幻读（Phantom Read） 
	=================================================================================================================
	
	未提交读（Read uncommitted）        YES                          YES                             YES
	
	已提交读（Read committed）          NO                           YES                             YES
	
	可重复读（Repeatable read）         NO                           NO                              YES
	
	可串行化（Serializable ）           NO                           NO                              NO
	
	=================================================================================================================

**死锁**：两个或多个事务同时请求对方占用的资源就会造成死锁，eg:
	
	事务A：                                                   事务B：
	start transaction                                        start transaction
	update t1 set value=1 where id=1                         update t2 set value=1 where id=2
	update t2 set value=2 where id=2                         update t1 set value=2 where id=1
	commit                                                   commit

在执行第二个update时就会死锁。 为解决死锁问题，数据库系统采用死锁检测和死锁超时机制，InnoDB目前处理死锁的办法是将持有最少行级排他锁的事务进行回滚。

**MyISAM与Innodb存储引擎区别：**

在MySQL 5.5版本前，默认的存储引擎为MyISAM。在那之后MySQL的默认存储引擎改为InnoDB。
	
1. MyISAM不支持事务，InnoDB支持事务。由于MyISAM在很长一段时间内是MySQL的默认存储引擎，所以在很多人的印象中MySQL是不支持事务的数据库。实际上，InnoDB是一个性能良好的事务性引擎。它实现了四个标准的隔离级别，默认的隔离级别为可重复读（REPEATABLE READ），并通过间隙锁策略来防止幻读的出现。此外它还通过多版本并发控制（MVCC）来支持高并发。

2. 对表的行数查询的支持不同。MyISAM内置了一个计数器来存储表的行数。执行 select count(*) 时直接从计数器中读取，速度非常快。而InnoDB不保存这些信息，执行 select count(*)需要全表扫描。当表中数据量非常大的时候速度很慢。
 
3. 锁的粒度不同。MyISAM仅支持表锁。每次操作锁住整张表。这种处理方式一方面加锁的开销比较小，且不会出现死锁，但另一方面并发性能较差。InnoDB支持行锁。每次操作锁住一行数据，一方面行级锁在每次获取锁和释放锁的操作需要消耗比表锁更多的资源，速度较慢，且可能发生死锁，但是另一方面由于锁的粒度较小，发生锁冲突的概率也比较低，并发性较好。此外，即使是使用了InnoDB存储引擎，但如果MySQL执行一条sql语句时不能确定要扫描的范围，也会锁住整张表。

4. 对主键的要求不同。MyISAM允许没有主键的表存在。而如果在建表时没有显示的指定主键，InnoDB就会为每一行数据自动生成一个6字节的ROWID列，并以此做为主键。这种主键对用户不可见。InnoDB对主键采取这样的策略是与它的数据和索引的组织方式有关的，下文会讲到。

5. 数据和索引的组织方式不同。MyISAM将索引和数据分开进行存储。索引存放在.MYI文件中，数据存放在.MYD文件中。索引中保存了相应数据的地址。以表名+.MYI文件分别保存。 InnoDB的主键索引树的叶子节点保存主键和相应的数据。其它的索引树的叶子节点保存的是主键。也正是因为采取了这种存储方式，InnoDB才强制要求每张表都要有主键。

6. 对AUTO_INCREMENT的处理方式不一样。如果将某个字段设置为INCREMENT，InnoDB中规定必须包含只有该字段的索引。但是在MyISAM中，也可以将该字段和其他字段一起建立联合索引。

7. delete from table的处理方式不一样。MyISAM会重新建立表。InnoDB不会重新建立表，而是一行一行的删除。因此速度非常慢。推荐使用truncate table，不过需要用户有drop此表的权限。

8. MyISAM崩溃后无法安全恢复，InnoDB支持崩溃后的安全恢复。InnoDB实现了一套完善的崩溃恢复机制，保证在任何状态下(包括在崩溃恢复状态下)数据库挂了，都能正常恢复。

9. MyISAM不支持外键，InnoDB支持外键。

10. 缓存机制不同。MyISAM仅缓存索引信息，而不缓存实际的数据信息。而InnoDB不仅缓存索引信息，还会缓存数据信息。其将数据文件按页读取到缓冲池，然后按最近最少使用的算法来更新数据。

**关于Schema与数据类型的优化：**

**选择标识符：** 整数类型通常是最好选择，尽量避免字符串类型做为标识符，因为需要消耗更多的空间，通常比整数类型慢。尤其是对于使用MD5()、SHA1()、UUID()等生成的随机字符串，会任意分布在很大的空间内导致insert和select变得很慢：

- 因为插入值会随机写到索引的不同位置，会导致页分裂、磁盘随机访问、索引碎片等；
- select会变得更慢，因为逻辑上相邻的行会分布在磁盘和内存的不同地方；
- 随机值导致缓存对所有类型的查询语句效果都很差，因为会使得缓存赖以工作的访问局部性原理失效。如果存储UUID值应该移除“-”符号，更好的做法是通过UNHEX()转换为16字节的数字并存储在BINARY(16)中。

**计数器表：**

	//只用一个计数器
	create table hit_counter(cnt int unsigned not null)
每次更新时都会加锁，并发性差，可以拆分成多行

	create table hit_counter(
		slot tinyint unsigned not null primary key
		cnt int unsigned not null
	)
预先增加100行数据，更新时：

	update hit_counter set cnt = cnt+1 where slot = RAND() * 100

查询：
	
	select sum(cnt) from hit_counter

**ALTER TABLE：**
	
	// 将film表的租赁期限默认值从3天改为5天
	ALTER TABLE film MODIFY COLUMN rental_duration TIMYINT(3) NOT NULL DEFAULT 5

该语句将拷贝整个表到新的表，MODIFY COLUMN都会重建表，可以使用ALTER COLUMN避免重建表

	ALTER TABLE film ALTER COLUMN rental_duration SET DEFAULT 5

列的默认值实际是存在.frm文件，该语句会直接修改.frm文件而不涉及表数据。

**索引：** innodb索引采用B+ Tree数据结构，叶子节点会保存数据和指向下一个叶子节点的指针（用于范围查询)，适用于全键值、
键值范围或键前缀查找，其中键前缀查找只适用于根据最左前缀查找。

	CREATE TABLE People(
		last_name varchar(50) not null,
		first_name varchar(50) not null,
		birthday date not null,
		key(last_name, first_name, birthday)
	);

索引查询的限制：

- 如果不是按照索引最左列开始查找则无法适用索引，如单独使用first_name或birthday查询将无法使用索引;
- 不能跳过索引中的某一列，如使用last_name和birthday进行查询时将只能使用索引中的第一列；
- 如果查询中有某个列的范围查询则无法使用该列右边的列，如: SELECT * FROM People WHERE last_name='WANG' and first_name LIKE 'Bob%' and birthday='1976-02-03'将只使用last_name和first_name两列
