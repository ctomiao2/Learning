**nohup**: Xshell连接服务器执行程序前加nohup可以避免断开连接程序被杀掉问题

**top**: 查看进程

**ps -u 用户名**: 查看用户的所有进程

**pwd**: 显示当前目录

**lls**：列出当前目录所有文件

**xshell sftp使用**：
	
	1）get：从远程服务器上下载一个文件存放到本地
	>> lcd d:\            #表示切换到本地的d盘下
	>> get ./test.sql　　 #这样就将当前文件下载本地的d盘下

	2）put：将本地的文件上传到远程服务器上
	>> put               #在windows下弹出选择文件的窗口
	
	3）lcd：先通过lcd切换到本地那个目录下
	>> lcd c:\            #表示切换到本地的c盘下

**netstat -tulpn:** 查看本地服务器端口映射

**grep -nr pattern**：查询当前目录下包含pattern的所有文件并显示行号
**gzip -dc myfile.gz | grep abc**：grep gz文件