Document：[https://docs.mongodb.com/manual/core](https://docs.mongodb.com/manual/core)

**CRUD:**

create： db.collection.insertOne(dict)、db.collection.insertMany(dict)

read：db.collection.find(query, projection).limit(limit_num)

update：db.collection.updateOne(filter, action)、db.collection.updateMany(filter, action)

delete：db.collection.deleteOne、db.collection.deleteMany

bulk write：db.collection.bulkWrite，打包多个写入操作，分ordered和unordered两种，ordered-bulk write后面的write操作要等前面的write执行完毕才能开始，unordered-bulk write则并行写入，写入过程中遇到错误时，ordered-bulk write会中断执行，unordered-bulk write则会继续执行后面的write operations

**mongodb与mysql类比：** [https://docs.mongodb.com/manual/reference/sql-comparison/](https://docs.mongodb.com/manual/reference/sql-comparison/)

**索引：** db.collection.createIndex( { field_1: 1或-1, field_2: 1或-1 } )，1是asc, -1是desc, 字段必须是string或string数组类型

**文本检索text search：** 使用'text'索引和$text操作符，eg: 

    db.stores.insert(
   	[
     { _id: 1, name: "Java Hut", description: "Coffee and cakes", price:80, count:100 },
     { _id: 2, name: "Burger Buns", description: "Gourmet hamburgers", price:85, count:521 },
     { _id: 3, name: "Coffee Shop", description: "Just coffee", price:60, count:1000 },
     { _id: 4, name: "Clothes Clothes Clothes", description: "Discount clothing", price:55, count:5000 },
     { _id: 5, name: "Java Shopping", description: "Indonesian goods", price: 60, count:50}
   	])

	db.stores.createIndex({ name: "text", description: "text" })
	db.stores.find( { $text: { $search: "java coffee shop \"middle school\"" } } ) //该语句将查询name或description包含java、coffe、shop、middle school其中任何一个的记录,
	db.stores.find( { $text: { $search: "java shop -coffee" } } ) //该语句将查询name或description包含java或shop但不包含coffee的记录

text search默认不排序，可以使用$meta："textScore"按匹配相关度排序，eg：

	db.stores.find(
   		{ $text: { $search: "java coffee shop" } },
   		{ score: { $meta: "textScore" } }
	).sort( { score: { $meta: "textScore" } } )
	
	返回结果：
	{ _id: 1, name: "Java Hut", description: "Coffee and cakes", price:80, count:100, score: 2.6},
	{ _id: 3, name: "Coffee Shop", description: "Just coffee", price:60, count:1000, score: 1.8},
	{ _id: 5, name: "Java Shopping", description: "Indonesian goods", price: 60, count:50, score: 1.4}

**注意：** 视图不支持text search，一个collection只能有一个text index，但一个text index可以覆盖多个字段

**聚合管道中使用文本检索：**
	
	//$match 选择price大于70且小于90，或者count大于等于1000
	db.stores.aggregate( [
	  { $match: { $or: [ { price: { $gt: 70, $lt: 90 } }, { count: { $gte: 1000 } } ] } },
	  { $group: { _id: null, count: { $sum: 1 } } }
	] );
	返回结果：{ "_id" : null, "count" : 4 }
	
	//文本搜索coffee|cake，搜索结果只保留_id、price字段且相关度大于1.0的documents
	db.stores.aggregate(
	[
	     { $match: { $text: { $search: "coffee cake" } } },
	     { $project: { _id: 1, price: 1, score: { $meta: "textScore" } } },
	     { $match: { score: { $gt: 1.0 } } }
	])
	// 搜索语言为英文, 返回搜索结果的price和
	db.stores.aggregate(
  	[
    	{ $match: { $text: { $search: "coffee -cakes", $language: "es" } } },
     	{ $group: { total_price: { $sum: "$price" } } }
    ])

**地理位置查询：** 使用GeoJson数据结构， def:

	location: {
      type: "Point",
      coordinates: [-73.856077, 40.848447]
	}

其中type为GeoJson object type，包括Point、MultiPoint、LineString、MultiLineString、Polygon、MultiPolygon、GeometryCollection。参考[https://docs.mongodb.com/manual/reference/geojson/](https://docs.mongodb.com/manual/reference/geojson/)

Geospatial Indexes：
	// 创建索引
	db.collection.createIndex( { <location field> : "2dsphere" }) // 类似地球的经纬度索引
	db.collection.createIndex( { <location field> : "2d" }) // 平面空间索引

查询函数：

- $geoNear
- $near、$nearSphere (>= MongoDB 4.0)
- $geoWithin
- $geoIntersect

以上支持sharded collections

eg: 

	db.places.insert( {
	    name: "Central Park",
	   location: { type: "Point", coordinates: [ -73.97, 40.77 ] },
	   category: "Parks"
	} );
	db.places.insert( {
	   name: "Sara D. Roosevelt Park",
	   location: { type: "Point", coordinates: [ -73.9928, 40.7193 ] },
	   category: "Parks"
	} );
	db.places.insert( {
	   name: "Polo Grounds",
	   location: { type: "Point", coordinates: [ -73.9375, 40.8303 ] },
	   category: "Stadiums"
	} );

	// 创建地理位置索引
	db.places.createIndex( { location: "2dsphere" } )
	
	// 查询距离目标位置1000m到5000m之间的documents，查询结果按距离从小到大排序
	db.places.find(
	{
	     location:
	       { $near:
	          {
	            $geometry: { type: "Point",  coordinates: [ -73.9667, 40.78 ] },
	            $minDistance: 1000,
	            $maxDistance: 5000
	          }
	       }
	})
	
	// 查询category是Parks，查询结果按与目标点的距离从小到大排序
	db.places.aggregate( [
	{
	      $geoNear: {
	         near: { type: "Point", coordinates: [ -73.9667, 40.78 ] },
	         spherical: true,
	         query: { category: "Parks" },
	         distanceField: "calcDistance"
	      }
	   }
	])


**读策略**：

1. readPreference：主要控制客户端driver从复制集的哪个节点读取，可方便地实现读写分离、就近读取等策略
	- primary： 只从primary节点读，默认设置
	- primaryPreferred：优先从primary读，primary不可用才从secondary读
	- secondary：只从secondary读
	- secondaryPerferred：优先从secondary读，没有secondary成员时才从primary读
	- nearest：根据网络距离就近读取<p>
	
2. readConcern：决定在某个实例读取时能读到什么数据
	- local：能读取任意数据，默认设置
	- majority：只能读到成功写入到大多数节点的数据
	
readConcern旨在解决脏读问题，比如在某个节点读到了一条数据，该数据还没被写入到大多数节点，如果这时primary发生故障，则重启后会rollback掉这条数据，从而此前按local策略读到的数据就是脏数据。
注意：majority只保证读到的数据不会发生回滚，但并不保证读到的数据是最新的，事实上客户端只会从某个确定的节点读取数据（具体哪个节点由readPreference决定），然后根据该节点的当前状态视图选择已写入到大多数节点的数据。
<p>
readConcern实现原理：mongodb要支持majority readConcern级别必须打开replication.enableMajorityReadConcern参数，打开参数后会开启一个snapshot线程，定期对当前数据进行snapshot获得最新的oplog映射表：
	
	最新OPLOG时间戳	SNAPSHOT				状态
	t0				snapshot0				committed
	t1				snapshot1				uncommitted
	t2				snapshot2				uncommitted
	t3				snapshot3				uncommitted

只有确保oplog已经同步到大多数节点了状态才会被标记为committed，客户端读取状态是committed的最新snapshot就能保证读到已写入到绝大多数节点的数据。如何确定oplog已经同步到了大多数节点？

- primary：secondary会在自身oplog发生变化时以及在发送的心跳信息里带上oplog通知到primary，从而primary能很快知道数据是否已被写入到绝大多数节点并更新committed状态，不必要的snapshot也会被定期清理掉。
- secondary：从primary拉取oplog时，primary会将最新的已写入到绝大多数节点的oplog信息返回给secondary，secondary根据oplog更新自己的snapshot状态。注意：**MongoDB的复制是通过secondary不断拉取oplog并重放来实现的，并不是primary主动将写入同步给secondary**

注意事项：

- 使用 readConcern 需要配置replication.enableMajorityReadConcern选项
- 只有支持 readCommited 隔离级别的存储引擎才能支持 readConcern，比如 wiredtiger 引擎，而 mmapv1引擎则不能支持。

**writeConcern：**

eg: db.collection.insert({x: 1}, {writeConcern: {w: 1}})

writeConcern选项：

1. w：数据写入到number个节点才向客户端确认：

	- {w: 0}：不需要任何确认，使用于性能要求高不关注正确性的场景。
	- {w: 1}：默认参数，数据写入到primary节点就返回确认。
	- {w: n}：n > 1，数据写入到primary且n-1个secondary就返回确认。
	- {w: "majority"}：数据写入到绝大多数节点才返回确认，对性能有影响，使用于对安全性要求较高的场景。

2. j：写入journal持久化了才通知确认，默认是false。
3. wtimeout：写入超时时间，仅w大于1时才有效。

{w: “majority”}解析：

1. 客户端向primary请求写入，primary收到请求后本地写入数据并记录写请求到oplog，然后等待绝大多数节点都同步了这条\批oplog（secondary更新了oplog后会上报给primary）
2. secondary拉取到primary新写入的oplog，本地重放并记录oplog。为了能让secondary尽快拉取到最新的oplog，find提供了awaitData的选项，当没有查到数据时会等待maxTimeMS（默认2s）后再看是否有满足条件的数据，如有则会返回，因此当primary有最新写入oplog，secondary能很快拉取到。
3. secondary上有自己的单独线程，当oplog最新的时间戳发生变化时就会将自己最新的oplog时间戳通过replSetUpdatePosition命令发送给primary去更新primary上记录的该secondary已完成同步的oplog时间戳。
4. 当primary发现已经有足够多的secondary的oplog时间戳满足条件时就会返回确认结果给客户端。