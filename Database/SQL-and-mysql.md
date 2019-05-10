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