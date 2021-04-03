#include"db.hpp"
#include"httplib.h"

using namespace httplib;

blog_system::TableBlog *table_blog;
blog_system::TableTag *table_tag;
//具体对应的业务处理
void InsertBlog(const Request &req, Response &rsp)
{
  //从请求中取出正文-正文就是提交的博客信息，以json格式的字符串形式组织的
  //将json格式的字符串进行反序列化，得到各个博客信息
  Json::Reader reader;
  Json::Value blog;
  Json::FastWriter writer;
  Json::Value errmsg;
  bool ret = reader.parse(req.body,blog);
  if(ret == false){
  rsp.status = 400;
  errmsg["ok"] = false;
  errmsg["reason"] = "parse blog json failed!";
  rsp.set_content(writer.write(errmsg),"application/json");
  return;
  }
//将得到的博客信息插入到数据库中；
  ret = table_blog->Insert(blog);
  if(ret == false){
    printf("InsertBlog insert into database failed!\n");
    rsp.status = 500;
  }
  rsp.status = 200;
  return;
}

void DeleteBlog(const Request &req, Response &rsp)
{
  // /blog/123   /blog/(\d+)  req.matches[0] = /blog/123   req.matches[1] = 123;
  int blog_id = std::stoi(req.matches[1]);
  bool ret = table_blog->Deletc(blog_id);
  if(ret == false){
    printf("DeletcBlog delect from database failed!\n");
    rsp.status = 500;
    return;
  }
  rsp.status = 200;
  return;
}

void UpdateBlog(const Request &req, Response &rsp)
{
  int blog_id = std::stoi(req.matches[1]);
  Json::Value blog;
  Json::Reader reader;
  bool ret = reader.parse(req.body,blog);
  if(ret == false){
     printf("UpdateBlog parse json failed!\n");
     rsp.status = 400;
     return;
  }
  blog["id"] = blog_id;
  ret = table_blog->Update(blog);
  if(ret == false){
    printf("UpdateBlog update from database failed!\n");
    rsp.status = 500;
    return;
  }
  rsp.status = 200;
  return;
}

void GetAllBlog(const Request &req, Response &rsp)
{
 //从数据库中取出博客列表；
 Json::Value blogs;
 bool ret = table_blog->GetAll(&blogs);
 if(ret == false){
   printf("GetAllBlog select from database failed!\n");
   rsp.status = 500;
   return;
 }
 //将数据进行json序列化，添加到rsp正文中；
 Json::FastWriter writer;
 rsp.set_content(writer.write(blogs),"application/json");
 rsp.status = 200;
 return;
}

void GetOneBlog(const Request &req, Response &rsp)
{
  int blog_id = std::stoi(req.matches[1]);
  Json::Value blog;
  blog["id"] = blog_id;
  bool ret = table_blog->GetOne(&blog);
  if(ret == false){
    printf("GetOneBlog select from database failed!\n");
    rsp.status = 500;
    return;
  }
 //将数据进行json序列化，添加到rsp正文中；
 Json::FastWriter writer;
 rsp.set_content(writer.write(blog),"application/json");
 rsp.status = 200;
 return;
}

void InsertTag(const Request &req, Response &rsp)
{
  Json::Reader reader;
  Json::Value tag;
  Json::FastWriter writer;
  Json::Value errmsg;
  bool ret = reader.parse(req.body,tag);
  if(ret == false){
  rsp.status = 400;
  return;
  }
  ret = table_tag->Insert(tag);
  if(ret == false){
    printf("InsertTag insert into database failed!\n");
    rsp.status = 500;
  }
  rsp.status = 200;
  return;
}

void DeleteTag(const Request &req, Response &rsp)
{
  int tag_id = std::stoi(req.matches[1]);
  bool ret = table_tag->Delete(tag_id);
  if(ret == false){
     printf("DeleteTag dalete from database failed!\n");
     rsp.status = 500;
     return;
  }
  rsp.status = 200;
  return;
}

void UpdateTag(const Request &req, Response &rsp)
{
  int tag_id = std::stoi(req.matches[1]);
  Json::Value tag;
  Json::Reader reader;
  bool ret = reader.parse(req.body,tag);
  if(ret == false){
    printf("UpdateTag parse json failed!\n");
    rsp.status = 400;
    return;
  }
  tag["id"] = tag_id;
  ret = table_tag->Update(tag);
  if(ret == false){
    printf("UpdateTag update database failed!\n");
    rsp.status = 500;
  }
  rsp.status = 200;
  return;
}

void GetAllTag(const Request &req, Response &rsp)
{
  Json::Value tags;
  bool ret = table_tag->GetAll(&tags);
  if(ret == false){
    printf("GetAllTag select from database failed!\n");
    rsp.status = 500;
    return;
  }
  Json::FastWriter writer;
  rsp.set_content(writer.write(tags),"application/json");
  rsp.status = 200;
  return;
}

void GetOneTag(const Request &req, Response &rsp)
{
  int tag_id = std::stoi(req.matches[1]);
  Json::Value tag;
  tag["id"] = tag_id;
  bool ret = table_tag->GetOne(&tag);
  if(ret == false){
    printf("GetOneTag select from database failed!\n");
    rsp.status = 500;
    return;
  }
 //将数据进行json序列化，添加到rsp正文中；
 Json::FastWriter writer;
 rsp.set_content(writer.write(tag),"application/json");
 rsp.status = 200;
  return;
}

int main()
{
  MYSQL *mysql = blog_system::MysqlInit();

  table_blog = new blog_system::TableBlog(mysql);
  table_tag = new blog_system::TableTag(mysql);
//业务处理模块
   Server server;
   //设置相对根目录的目的：当客户端请求静态文件资源时，httplib会直接根据跟路径读取文件数据进行响应;
   // /index.html   ->      ./www/index.html ;
   server.set_base_dir("./www");//设置url中的资源相对根目录，并且在请求/的时候，自动添加index.html;
   //博客信息的增删查改
   server.Post("/blog",InsertBlog);
   //正则表达式：\d-匹配数字字符  +--表示匹配前边的字符一次或多次，()为了临时保存匹配的数据;
   //  /blog/(\d+) 表示匹配 以/blog/开头，后边跟了一个数字的字符串格式，并且临界保存后面的数字
   server.Delete(R"(/blog/(\d+))",DeleteBlog);//R"()"取出括号中所有字符的特殊含义;
   server.Put(R"(/blog/(\d+))",UpdateBlog);
   server.Get("/blog",GetAllBlog);
   server.Get(R"(/blog/(\d+))",GetOneBlog);
  
   //标签信息的增删查改
   server.Post("/tag",InsertTag);
   server.Delete(R"(/tag/(\d+))",DeleteTag);//R"()"取出括号中所有字符的特殊含义;
   server.Put(R"(/tag/(\d+))",UpdateTag);
   server.Get("/tag",GetAllTag);
   server.Get(R"(/tag/(\d+))",GetOneTag);
   server.listen("0.0.0.0",10007);
  return 0;
}


/*void test()
{

   MYSQL *mysql = blog_system::MysqlInit();
   blog_system::TableBlog table_blog(mysql);
   //博客的增删改查功能测试：
   Json::Value blog;
   blog["id"] = 1;
   //blog["tag_id"] = 4;
   //blog["title"] = "这个是Linux博客";
   //blog["content"] = "wo ai xue xi!";
   //table_blog.Insert(blog);
   //table_blog.Deletc(4);
   //table_blog.Update(blog);
   table_blog.GetOne(&blog);
   Json::StyledWriter writer;
   std::cout << writer.write(blog) << std::endl;
   */



  /* 标签的增删改查功能测试：
   blog_system::TableTag table_tag(mysql);
   Json::Value tag;
   //tag["name"] = "Python";
   tag["id"] = 1;
   //table_tag.Insert(tag);
   //table_tag.Update(tag);
   table_tag.GetOne(&tag);
   Json::StyledWriter writer;
   std::cout << writer.write(tag) << std::endl;

   return 0;
}*/
