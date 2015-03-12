
/**
 *
 * Author  : D.Dinesh
 *           www.techybook.com
 *           dinesh@techybook.com
 *
 * Licence : Refer the license file
 *
 **/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/stat.h>

#include "Icejson.h"

#define OK   true
#define ERR  false

#define SKIP_WHITE_SPACE(str, line, line_bgn)   \
({                                              \
   for( ; IS_SPACE(*str); str++)                \
      if('\n' == *str)                          \
      {                                         \
         line++;                                \
         line_bgn = str + 1;                    \
      }                                         \
})

#define IS_SPACE(ch) (' ' == ch or '\t' == ch or '\r' == ch or '\n' == ch)

struct Exception
{
   int line;
   const char *fn;
   const char *file;
   const char *msg;

   Exception(const char *file, const char *fn, size_t line, const char *msg) :
      file(file), fn(fn), line(line), msg(msg) {}
};

#define trw_err(msg) throw Exception(__FILE__, __FUNCTION__, __LINE__, msg)

enum Symbol
{
   LEX_INT              = 'I'    ,
   LEX_NEG              = '-'    ,
   LEX_FLOAT            = '.'    ,
   LEX_STRING           = '"'    ,
   LEX_NULL             = 'N'    ,
   LEX_BOOL_TRUE        = 'T'    ,
   LEX_BOOL_FALSE       = 'F'    ,

   LEX_ARRAY_OPEN       = '['    ,
   LEX_ARRAY_CLOSE      = ']'    ,

   LEX_OBJECT_OPEN      = '{'    ,
   LEX_OBJECT_CLOSE     = '}'    ,

   LEX_NAME_SEPERATOR   = ':'    ,
   LEX_VALUE_SEPERATOR  = ','    ,

   LEX_INVALID          = 0x00
};

struct Lexer_t
{
   Lexer_t();

   Symbol cur_sym;
   const char *cur_pos;
   const char *json_str;

   int line;
   const char *line_bgn;

   void load_string(const char *json_arg);

   Symbol next();
   Symbol get_sym();

   Symbol get_str(std::string &val);
   Symbol get_num(const char * &val);
   Symbol get_char(const char * &val);
};

Lexer_t::Lexer_t()
{
   line = 1;
   line_bgn = NULL;
   cur_pos = NULL;
   json_str = NULL;
}

void Lexer_t::load_string(const char *json_arg)
{
   line_bgn = json_str = cur_pos = json_arg;
   get_sym(); /* this will set cur_sym */
}

Symbol Lexer_t::next()
{
   cur_pos++;
   return get_sym();
}

Symbol Lexer_t::get_sym()
{
   SKIP_WHITE_SPACE(cur_pos, line, line_bgn);
   char ch = *cur_pos;
   switch(ch)
   {
      case LEX_NEG             :
      case LEX_STRING          :
      case LEX_ARRAY_OPEN      :
      case LEX_ARRAY_CLOSE     :
      case LEX_OBJECT_OPEN     :
      case LEX_OBJECT_CLOSE    :
      case LEX_NAME_SEPERATOR  :
      case LEX_VALUE_SEPERATOR : cur_sym = Symbol(ch);
                                 break;

      default  : if('0' <= ch and ch <= '9')
                    cur_sym = LEX_INT;
                 else if(0 == strncmp(cur_pos, "true", 4))
                 {
                    cur_sym  = LEX_BOOL_TRUE;
                    cur_pos += 3;
                 }
                 else if(0 == strncmp(cur_pos, "false", 5))
                 {
                    cur_sym  = LEX_BOOL_FALSE;
                    cur_pos += 4;
                 }
                 else if(0 == strncmp(cur_pos, "null", 4))
                 {
                    cur_sym  = LEX_NULL;
                    cur_pos += 3;
                 }
                 else cur_sym = LEX_INVALID;
   }
   return cur_sym;
}

Symbol Lexer_t::get_num(const char * &val)
{
   val = cur_pos;
   Symbol sym = LEX_INT;

   if('-' == *cur_pos and !isdigit(*++cur_pos))
         trw_err("Expected digit");

   while(isdigit(*cur_pos)) 
      cur_pos++;

   if('.' == *cur_pos && isdigit(cur_pos[1]))
   {
      sym = LEX_FLOAT;
      while(isdigit(*++cur_pos)); 
   }

   if('e' == *cur_pos || 'E' == *cur_pos)
   {
      sym = LEX_FLOAT;
      if(isdigit(*++cur_pos))
         while(isdigit(*++cur_pos));
      else if('+' == *cur_pos || '-' == *cur_pos)
      {
         if(not isdigit(*++cur_pos))
            trw_err("Expected digit");
         while(isdigit(*++cur_pos));
      }
   }

   get_sym();

   return sym;
}

Symbol Lexer_t::get_str(std::string &val)
{
   cur_pos++;           /* skip string symbol */
   val.clear();
   char arr[65] = {};

   for(int I = 0; '"' != *cur_pos; I++, cur_pos++)
   {
      if(I >= 64)
      {
         I = 0;
         val += arr;
         memset(arr, 0, sizeof arr);
      }

      if('\n' == *cur_pos)
      {
         line++;
         line_bgn = cur_pos + 1;
      }
      
      if('\\' == *cur_pos)
      {
         switch(*++cur_pos)
         {
            case '"'  : arr[I] = '"' ; break;
            case '\\' : arr[I] = '\\'; break;
            case '/'  : arr[I] = '/' ; break;
            case 'b'  : arr[I] = '\b'; break;
            case 'f'  : arr[I] = '\f'; break;
            case 'n'  : arr[I] = '\n'; break;
            case 'r'  : arr[I] = '\r'; break;
            case 't'  : arr[I] = '\t'; break;

            case 'u'  : arr[I] = 0;
                        for(int J = 0; J < 4 && cur_pos++; J++)
                        {
                           arr[I] <<= 4;
                           char ch = *cur_pos;
                           if('0' <= ch and ch <= '9')
                              arr[I] |= (ch - '0');
                           else if('a' <= ch and ch <= 'f')
                              arr[I] |= (ch - 'a' + 0xA);
                           else if('A' <= ch and ch <= 'F')
                              arr[I] |= (ch - 'A' + 0xA);
                           else trw_err("Invalid unicode value");
                        }
                        break;

            default   : trw_err("Invalid escape sequence");
         }
      }
      else
      {
         arr[I] = *cur_pos;
      }
   }
   
   val += arr;

   return get_sym();
}


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~.
 |        Parser related implementations starts      |
 `~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
namespace Icejson
{
   #define oInvalid (*((Node_t *)0))

   #define NEXT_NEW_NODE(ptr) \
   ({ \
         Parser_t *swp = new Parser_t(pdoc); \
         swp->pprev = pp; \
         pp->pnext = pp = swp; \
         pp->pparent = this; \
    })

   struct Parser_t : public Node_t
   {
      Parser_t(Doc_t *doc) { pdoc = doc; }

      Parser_t(Doc_t *doc, Valtype_t type) 
      { 
         pdoc = doc;
         vtype = type; 
      }

      bool ParseArray(Lexer_t &lex);
      bool ParseObject(Lexer_t &lex);
      bool ParseNode(Lexer_t &lex, Symbol node_close);
   };

   bool Parser_t::ParseNode(Lexer_t &lex, Symbol node_close)
   {
      switch(lex.cur_sym)
      {
         case LEX_NEG         : 
         case LEX_INT         : const char *val;
                                if(LEX_INT == lex.get_num(val))
                                {
                                   vint = atoi(val);
                                   vtype = Valtype::Int;
                                }
                                else 
                                {
                                   vreal = atof(val);
                                   vtype = Valtype::Float;
                                }
                                break;

         case LEX_STRING      : lex.get_str(vstr);
                                if(LEX_STRING != lex.cur_sym)
                                {
                                   trw_err("Unterminated string value");
                                }
                                else 
                                { 
                                   lex.next(); 
                                   vtype = Valtype::String; 
                                   break; 
                                }

         case LEX_BOOL_TRUE   : vbool = true;
                                vtype = Valtype::Bool;
                                lex.next();
                                break;
         
         case LEX_BOOL_FALSE  : vbool = false;
                                vtype = Valtype::Bool;
                                lex.next();
                                break;

         case LEX_NULL        : vtype = Valtype::Null;
                                lex.next();
                                break;

         case LEX_ARRAY_OPEN  : ParseArray(lex);
                                vtype = Valtype::Array;
                                lex.next(); /* move past array close symbol */
                                break;

         case LEX_OBJECT_OPEN : ParseObject(lex);
                                vtype = Valtype::Object;
                                lex.next(); /* move past object close symbol */
                                break;

         default : trw_err("Expected number, char, string, array or object");
      }

      if(LEX_VALUE_SEPERATOR != lex.cur_sym and 
            node_close != lex.cur_sym)
         trw_err("Expected value seperator");

      return OK;
   }

   bool Parser_t::ParseArray(Lexer_t &lex)
   {
      Parser_t *pp = NULL;

      /* handle empty array */
      if(LEX_ARRAY_CLOSE == lex.next())
         return OK;

      pcount++;
      vobj = pp = new Parser_t(pdoc);
      pp->pparent = this;
      pp->ParseNode(lex, LEX_ARRAY_CLOSE);

      while(LEX_ARRAY_CLOSE != lex.cur_sym)
      {
         pcount++;
         lex.next();
         NEXT_NEW_NODE(pp);
         pp->ParseNode(lex, LEX_ARRAY_CLOSE);
      }

      vlast = pp;

      return OK;
   }

   bool Parser_t::ParseObject(Lexer_t &lex)
   {
      Parser_t *pp = NULL;
      vobj = pp = new Parser_t(pdoc);

      while(LEX_OBJECT_CLOSE != lex.cur_sym)
      {
         /* handle empty objects */
         if(LEX_OBJECT_CLOSE == lex.next())
            break;

         if(LEX_STRING != lex.cur_sym)
            trw_err("Expected node name");

         lex.get_str(pp->name);
         if(LEX_STRING != lex.cur_sym)
            trw_err("Invalid node name");

         if(LEX_NAME_SEPERATOR != lex.next())
            trw_err("Expected name seperator");
         
         pcount++;
         lex.next();
         pp->pparent = this;
         pp->ParseNode(lex, LEX_OBJECT_CLOSE);
         NEXT_NEW_NODE(pp);
      }

      if(vobj == pp) /* object is empty */
      {
         vobj = NULL;
      }
      else
      {
         vlast = pp->pprev;
         vlast->pnext = NULL;
      }
      delete pp;

      return OK;
   }
}


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~.
 |        Document related implementations starts      |
 `~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
namespace Icejson
{
   Doc_t::Doc_t() {}

   Node_t & Doc_t::root() { return *proot; }

   Node_t & Doc_t::parse_string(const char *json_arg)
   {
      Lexer_t lex;
      Parser_t *pp = NULL; /* pointer to parser */

      try
      {
         lex.load_string(json_arg);

         if(LEX_OBJECT_OPEN != lex.cur_sym)
            trw_err("Expected object at start");

         pp = new Parser_t(this, Valtype::Object);
         pp->ParseObject(lex);

         return *pp;
      }
      catch(Exception exc)
      {
         error.desc = exc.msg;
         error.line = lex.line;
         error.colum = lex.cur_pos - lex.line_bgn + 1;
         error.offset = lex.cur_pos - lex.json_str + 1;
      }

      return oInvalid;
   }

   Node_t & Doc_t::parse_file(FILE *fh)
   {
      struct stat st;
      char *json_str = NULL;
      fstat(fileno(fh), &st);
      json_str = new char [st.st_size];
      fread(json_str, sizeof(char), st.st_size, fh);
      Node_t &root = parse_string(json_str);
      delete [] json_str;
      return root;
   }

   Node_t & Doc_t::parse_file(const char *file_path)
   {
      FILE *fh = fopen(file_path, "r");
      if(NULL == fh) return oInvalid;
      Node_t &ret = parse_file(fh);
      fclose(fh);
      return ret;
   }
}


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~.
 |       Writer realted implementation            |
 `~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
namespace Icejson
{
   Writer_t::Writer_t() 
   {
      int_format = "%d";
      str_format = "\"%s\"";
      char_format = "'%c'";
      float_format = "%f";
   }

   int Node_t::write(FILE *fh, const char *pad, int level)
   {
      int len = 0;
      Node_t *itr = NULL;
      Writer_t &wrt = pdoc->writer;;

      for(int I = 0; I < level; I++)
         len += fprintf(fh, "%s", pad);

      if(not name.empty())
         len += fprintf(fh, "\"%s\" : ", name.data());

      switch(vtype)
      {
         case Valtype::Int : len += fprintf(fh, wrt.int_format.data(), vint); 
                             break;
         case Valtype::Char : len += fprintf(fh, wrt.char_format.data(), vchar); 
                              break;
         case Valtype::Float : len += fprintf(fh, wrt.float_format.data(), vreal); 
                               break;
         case Valtype::String : len += fprintf(fh, wrt.str_format.data(), vstr.data()); 
                                break;

         case Valtype::Array : len += fprintf(fh, "[");
                               if(vobj)
                               {
                                  fprintf(fh, "\n");
                                  for(itr = vobj; ; )
                                  {
                                     len += itr->write(fh, pad, level + 1);
                                     itr  = itr->pnext;
                                     if(itr) len += fprintf(fh, ",\n");
                                     else { len += fprintf(fh, "\n"); break; }
                                  }
                                  for(int I = 0; I < level; I++)
                                     len += fprintf(fh, "%s", pad);
                               }
                               len += fprintf(fh, "]");
                               break;

         case Valtype::Object : len += fprintf(fh, "{");
                                if(vobj)
                                {
                                   fprintf(fh, "\n");
                                   for(itr = vobj; ; )
                                   {
                                      len += itr->write(fh, pad, level + 1);
                                      itr = itr->pnext;
                                      if(itr) len += fprintf(fh, ",\n");
                                      else { len += fprintf(fh, "\n"); break; }
                                   }
                                   for(int I = 0; I < level; I++)
                                      len += fprintf(fh, "%s", pad);
                                }
                                len += fprintf(fh, "}");
                                break;

         case Valtype::Null : len += fprintf(fh, "null"); break;
      }

      return len;
   }
}


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~.
 |       Node related implementations starts      |
 `~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
namespace Icejson
{
   Node_t::Node_t()    
   {
      pcount = 0;

      pdoc = NULL;
      vobj = NULL;

      proot = NULL;
      pnext = NULL;
      pprev = NULL;
      pparent = NULL;

      vtype = Valtype::Invalid;
   }

   Iterator_t Node_t::front() const
   {
      if(this != &oInvalid and
        (Valtype::Array == vtype or
            Valtype::Object == vtype))
         return Iterator_t(vobj);
      return Iterator_t(NULL);
   }

   Iterator_t Node_t::back() const
   {
      if(this != &oInvalid and
        (Valtype::Array == vtype or
            Valtype::Object == vtype))
         return Iterator_t(vlast);
      return Iterator_t(NULL);
   }

   Doc_t & Node_t::doc() const     { return *pdoc;    }

   Node_t & Node_t::root() const   { return *proot;   }
   Node_t & Node_t::prev() const   { return *pprev;   }
   Node_t & Node_t::next() const   { return *pnext;   }
   Node_t & Node_t::parent() const { return *pparent; }

   int Node_t::count() const       { return pcount;   }

   bool Node_t::valid() const     { return this != &oInvalid; }
   Node_t::operator bool () const { return this != &oInvalid; }

   Valtype_t Node_t::value_type() const { return vtype; }

   Node_t::operator int    () const { return vint;  }
   Node_t::operator char   () const { return vchar; }
   Node_t::operator float  () const { return vreal; }
   Node_t::operator string () const { return vstr;  }

   Node_t & Node_t::operator [] (const int idx) const
   {
      if(Valtype::Array == vtype or 
            Valtype::Object == vtype)
      {
         Node_t *cur = vobj;
         for(int I = 0; cur && I < idx; I++)
            cur = cur->pnext;
         return *cur;
      }
      return oInvalid;
   }

   Node_t & Node_t::operator [] (const char *name) const
   {
      if(Valtype::Object == vtype)
      {
         Node_t *cur = vobj;
         for( ; cur; cur = cur->pnext)
            if(cur->name == name)
               return *cur;
      }
      return oInvalid;
   }
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~.
 |        Iterator related implementations starts      |
 `~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
namespace Icejson
{
   Iterator_t::Iterator_t() {}

   Iterator_t::Iterator_t(Node_t *node) :
      pcur(node) {}

   Iterator_t::operator bool()
   {
      return pcur;
   }

   Node_t & Iterator_t::operator * ()
   {
      return *pcur;
   }
   
   Iterator_t & Iterator_t::operator ++ ()
   {
      pcur = pcur->pnext;
      return *this;
   }

   Iterator_t & Iterator_t::operator -- ()
   {
      pcur = pcur->pprev;
      return *this;
   }

   Iterator_t Iterator_t::operator ++ (int)
   {
      Iterator_t itr(pcur);
      pcur = pcur->pnext;
      return itr;
   }

   Iterator_t Iterator_t::operator -- (int)
   {
      Iterator_t itr(pcur);
      pcur = pcur->pprev;
      return itr;
   }

   bool Iterator_t::operator == (const Iterator_t &rhs)
   {
      return pcur == rhs.pcur;
   }

   bool Iterator_t::operator != (const Iterator_t &rhs)
   {
      return pcur != rhs.pcur;
   }
}
