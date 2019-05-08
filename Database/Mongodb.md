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
	
