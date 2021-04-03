#include<iostream>
#include<mysql/mysql.h>
#include<jsoncpp/json/json.h>
#include<mutex>

#define MYSQL_HOST "127.0.0.1"
#define MYSQL_USER "root"
#define MYSQL_PSWD "2415357472"
#define MYSQL_DB  "wyb_db_blog" 


using namespace std;

namespace blog_system{

  static std::mutex g_mutex;

  MYSQL *MysqlInit(){
     MYSQL *mysql;
    //初始化
    mysql = mysql_init(NULL);
    if(mysql == NULL){
      printf("init mysql error\n");
      return NULL;
    }
    //连接服务器"8.131.79.37"
    //
    if(mysql_real_connect(mysql,MYSQL_HOST,MYSQL_USER,MYSQL_PSWD,MYSQL_DB,3306,NULL,0) == NULL){
      printf("connect mysql server error:%s\n",mysql_error(mysql));
      mysql_close(mysql);
      return NULL;
    }else{
      cout << "数据库连接成功！！"<<endl;
    }
    //设置字符集
    if(mysql_set_character_set(mysql,"utf8") != 0){
      printf("set client character error:%s\n",mysql_error(mysql));
      mysql_close(mysql);
      return NULL;
    }
    //选择数据库
    if(mysql_select_db(mysql,MYSQL_DB) != 0){
       printf("select db error:%s\n",mysql_error(mysql));
       mysql_close(mysql);
       return NULL;
    }
    return mysql;
  } //向外提供接口返回初始化完成的mysql句柄（连接服务器，选择数据库，设置字符集）;
  
  void MysqlRelease(MYSQL *mysql){
    if(mysql){
      mysql_close(mysql);
    }
    return;
  }  //销毁句柄;    
 
  bool MysqlQuery(MYSQL *mysql,const char *sql){
     int ret = mysql_query(mysql,sql);
     if(ret != 0){
       printf("query sql:[%s] failed:%s\n",sql,mysql_error(mysql));
       return false;
     }
     return true;

  } //执行语句的共有接口,封装一下就不用每次打印错误信息了；
  
  class TableBlog
  {
    public:
      TableBlog(MYSQL *mysql):_mysql(mysql){}
      bool Insert(Json::Value &blog)
      {
      //id tag_id title content ctime{tag_id:1,title:Linux博客，content：博客挺好看}
#define INSERT_BLOG "insert tb_blog values(null,'%d','%s','%s',now());"
        //因为正文长度不定，有可能会很长，如果直接定义tmp空间长度固定，有可能造成访问越界
        int len = blog["content"].asString().size() + 4096;
        char* tmp = (char*)malloc(len);
        sprintf(tmp,INSERT_BLOG,blog["tag_id"].asInt(), blog["title"].asCString(), blog["content"].asCString());
        bool ret = MysqlQuery(_mysql,tmp);
        free(tmp);
        return ret;
      } //从blog中取出博客信息，组织sql语句，将数据插入数据;
     
      bool Deletc(int blog_id){
#define DELETE_BLOG "delete from tb_blog where id = %d"
        char tmp[1024] =  {0};
        sprintf(tmp,DELETE_BLOG,blog_id);
        bool ret = MysqlQuery(_mysql,tmp);
        return ret;
      } //根据博客id删除博客;
      
      bool Update(Json::Value &blog){
        //id tag_id title content ctime{tag_id:1; title:linux博客 ,content:博客挺好看}
#define UPDATE_BLOG "update tb_blog set tag_id=%d,title='%s',content='%s'where id = %d;"
        int len = blog["content"].asString().size() + 4096;
        char * tmp = (char*)malloc(len);
        sprintf(tmp,UPDATE_BLOG,blog["tag_id"].asInt(), blog["title"].asCString(), blog["content"].asCString(), blog["id"].asInt());
        bool ret = MysqlQuery(_mysql,tmp);
        free(tmp);
        return ret;
      } //从blog中取出博客信息，组织sql语句，更新数据库中数据;
      
      bool GetAll(Json::Value *blogs){
#define GETALL_BLOG "select id,tag_id,title,ctime from tb_blog;"       
        //执行查询语句
        g_mutex.lock();
        bool ret = MysqlQuery(_mysql,GETALL_BLOG);
        if(ret == false){
          g_mutex.unlock();
          return false;
        }
        //保存结果集
        MYSQL_RES *res = mysql_store_result(_mysql);
        g_mutex.unlock();
        if(res == NULL){
          printf("store all blog result failde:%s\n",mysql_error(_mysql));
          return false;
        }
        //遍历结果集
        int row_num = mysql_num_rows(res);
        for(int i = 0; i < row_num;i++){
          MYSQL_ROW row = mysql_fetch_row(res);
          Json::Value blog;
          blog["id"] = std::stoi(row[0]);  //将字符串数据转化为整形数据--字符串转数字;
          blog["tag_id"] = std::stoi(row[1]);
          blog["title"] = row[2];
          blog["ctime"] = row[3];
          blogs->append(blog); //append添加json数组元素;
        }
        mysql_free_result(res);
        return true;
      } //通过blog返回所有的博客信息（因为通常是列表展示，所有不包括正文）;
      
      bool GetOne(Json::Value *blog){
#define GETONE_BLOG "select tag_id,title,content,ctime from tb_blog where id = %d;"
        char tmp[1024]={0};
        sprintf(tmp,GETONE_BLOG,(*blog)["id"].asInt());
        g_mutex.lock();
        bool ret = MysqlQuery(_mysql,tmp);
        if(ret == false){
          g_mutex.unlock();
          return false;
        }
        MYSQL_RES *res = mysql_store_result(_mysql);
        g_mutex.unlock();
        if(res == NULL)
        {
          printf("store all blog result failed:%s\n",mysql_error(_mysql));
          return false;
        }
        int row_num = mysql_num_rows(res);
        if(row_num != 1){
          printf("get one blog result error\n");
          mysql_free_result(res);
          return false;
        }
        MYSQL_ROW row = mysql_fetch_row(res);
        (*blog)["tag_id"] = std::stoi(row[0]);
        (*blog)["title"] = row[1]; 
        (*blog)["content"] = row[2]; 
        (*blog)["ctime"] = row[3];
        mysql_free_result(res);
        return true;   
      }  //返回单个博客，包括正文;
    private:
        MYSQL *_mysql;
  };

// 到此为所有的博客信息的操作代码;

//标签表
class TableTag
{
  public:
    TableTag(MYSQL *mysql):_mysql(mysql){}
 
    bool Insert(Json::Value &tag){
#define INSERT_TAG "insert tb_tag values(null,'%s');"
      char tmp[1024] = {0};
      sprintf(tmp,INSERT_TAG,tag["name"].asCString());
      return MysqlQuery(_mysql,tmp);
    }

    bool Delete(int tag_id){
#define DELETE_TAG "Delete from tb_tag where id = %d;"
      char tmp[1024] = {0};
      sprintf(tmp,DELETE_TAG,tag_id);
      return MysqlQuery(_mysql,tmp);
    }
   
    bool Update(Json::Value &tag){
#define UPDATE_TAG "update tb_tag set name = '%s' where id = %d;"
      char tmp[1024] = {0};
      sprintf(tmp,UPDATE_TAG,tag["name"].asCString(),tag["id"].asInt());
      return MysqlQuery(_mysql,tmp);
    }
    
    bool GetAll(Json::Value *tags){
#define GETALL_TAG "select id,name from tb_tag;"
      g_mutex.lock();
      bool ret = MysqlQuery(_mysql,GETALL_TAG);
      if(ret == false){
        g_mutex.unlock();
        return false;
      }
      MYSQL_RES *res = mysql_store_result(_mysql);
      g_mutex.unlock();
      if(res == NULL)
      {
        printf("store all tag result failed:%s\n",mysql_error(_mysql));
        return false;
      }
      int row_num = mysql_num_rows(res);
      for(int i = 0;i < row_num;i++){
        MYSQL_ROW row = mysql_fetch_row(res);
        Json::Value tag;
        tag["id"] = std::stoi(row[0]);
        tag["name"] = row[1];
        tags->append(tag);
      }
      mysql_free_result(res);
      return true;
    }
  
    bool GetOne(Json::Value *tag){
#define GETONE_TAG "select name from tb_tag where id = %d;"
      char tmp[1024] = {0};
      sprintf(tmp,GETONE_TAG,(*tag)["id"].asInt());
      g_mutex.lock();
      bool ret = MysqlQuery(_mysql,tmp);
      if(ret == false){
        g_mutex.unlock();
        return false;
      }
      MYSQL_RES *res = mysql_store_result(_mysql);
      g_mutex.unlock();
      if(res == NULL) {
        printf("store one tag result failed:%s\n",mysql_error(_mysql));
        return false;
      }
      int row_num = mysql_num_rows(res);
      if(row_num != 1){
        printf("get one tag result errpr\n");
        mysql_free_result(res);
        return false;
      }
      MYSQL_ROW row = mysql_fetch_row(res);
      (*tag)["name"] = row[0];
      mysql_free_result(res);
      return true;
    }
  private:
    MYSQL *_mysql;
 };
}
