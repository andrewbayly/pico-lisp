/**
TODO: 
  - goal - make a function which computes a factorial using recursion - DONE!
    - if
    - * 
  - implement back-space - MOSTLY DONE :)
  - show a cursor - DONE!
  - long string crashed program - DONE!
  - hitting return breaks the program completely! - DONE!
  - save/load/clear
  - functions don't appear e.g. (def! foo (fn* (a) a)) - just evaluates to blank
  - memory leaks
  - () should eval to nil
  - figure out how to get my code into github

**/

extern "C" int foo();
extern "C" void print(char ch); 
extern "C" void mal_key_pressed(char ch); 
extern "C" void mal_init(void);

#include <string>
#include <regex>
#include <vector>
#include <unordered_map>

using namespace std;

int foo(){ 
  try{ 
    throw 44; 
  }
  catch(int e){ 
  }
  return 0;
}

void mal_print(char ch){ 
  print(ch); 
}

void print_string(std::string s){ 
  for(unsigned int i = 0; i < s.length(); i++){ 
    mal_print(s[i]); 
  }
}



std::string input; 

bool is_balanced(std::string s){ 
  int count_open = 0; 
  int count_close = 0; 
  
  for(unsigned int i = 0; i < s.length(); i++){ 
    char ch = s[i]; 
    if(ch == '('){ 
      count_open++; 
    }
    if(ch == ')'){ 
      count_close++; 
    }
  }
  return (count_open == count_close);  
}

/**
void join(const vector<string>& v, char c, string& s) {

   s.clear();

   for (vector<string>::const_iterator p = v.begin();
        p != v.end(); ++p) {
      s += *p;
      if (p != v.end() - 1)
        s += c;
   }
}
**/

//=====================================================================================
//       READ   
//=====================================================================================

std::vector<std::string> split(const std::string &text, char sep) {
    std::vector<std::string> tokens;
    std::size_t start = 0, end = 0;
    while ((end = text.find(sep, start)) != std::string::npos) {
        if (end != start) {
          tokens.push_back(text.substr(start, end - start));
        }
        start = end + 1;
    }
    if (end != start) {
       tokens.push_back(text.substr(start));
    }
    return tokens;
}

#define NODE_LIST 0
#define NODE_BOOL 1
#define NODE_NUMBER 2
#define NODE_NIL 3
#define NODE_SYMBOL 4
#define NODE_FUNCTION 4

class Node { 

public: 
  Node(int _type){
    type = _type;  
  }
   
  ~Node(){
  } 

  string toString(){ 
    if(this->type == NODE_BOOL){ 
      if(this->value == 0)
        return string("false"); 
      else
        return string("true"); 
    }
    else if(this->type == NODE_SYMBOL){ 
      return this->symbol; 
    }
    else if(this->type == NODE_LIST){ 
      string s = "(";
      for(unsigned int i = 0; i < this->list.size(); i++){ 
        s += this->list[i]->toString();
        if(i < this->list.size()-1)
          s += " ";  
      } 
      s += ")"; 
      return s;
    }
    else if(this->type == NODE_NIL){ 
      return "nil"; 
    }
    else if(this->type == NODE_NUMBER){ 
      return to_string(this->value ); 
    }
    else if(this->type == NODE_FUNCTION){ 
      return string("<function>"); 
    }
    else
      return "ERROR!"; 
  }
  
  int type; 
  vector<Node*> list; 
  int value;
  string symbol; 
  
  virtual Node* call(vector<Node*>* args){
    return NULL;  
  }
  
  bool isTrue(){ 
    return !(type == NODE_NIL || (type == NODE_BOOL && value == 0)); 
  }
  
  bool isSpecial(string name){ 
    return type == NODE_SYMBOL && symbol.compare(name)==0; 
  }
}; 

class AddFunction : public Node { 

public: 
  AddFunction(): Node(NODE_FUNCTION)
  { 
  }   

  virtual Node* call(vector<Node*>* args){
    Node* pNode = new Node(NODE_NUMBER); 
    pNode->value = args->at(0)->value + args->at(1)->value; 
    return pNode;   
  }
};

class SubtractFunction : public Node { 

public: 
  SubtractFunction(): Node(NODE_FUNCTION)
  { 
  }   

  virtual Node* call(vector<Node*>* args){
    Node* pNode = new Node(NODE_NUMBER); 
    pNode->value = args->at(0)->value - args->at(1)->value; 
    return pNode;   
  }
};

class MultiplyFunction : public Node { 

public: 
  MultiplyFunction(): Node(NODE_FUNCTION)
  { 
  }   

  virtual Node* call(vector<Node*>* args){
    Node* pNode = new Node(NODE_NUMBER); 
    pNode->value = args->at(0)->value * args->at(1)->value; 
    return pNode;   
  }
};

class EqualsFunction : public Node { 

public: 
  EqualsFunction(): Node(NODE_FUNCTION)
  { 
  }   

  virtual Node* call(vector<Node*>* args){
    Node* pNode = new Node(NODE_BOOL); 
    pNode->value = ( args->at(0)->value == args->at(1)->value ) ? 1 : 0 ; 
    return pNode;   
  }
};


bool isNumber(string s)
{
  char* p;
  strtol(s.c_str(), &p, 10);
  return *p == 0;
}

Node* categorize(string s){ 
  //TODO: add nil and integer
 
  Node* pNode; 
 
  if(isNumber(s)){ 
    pNode = new Node(NODE_NUMBER); 
    pNode->value = stoi(s); 
    return pNode; 
  } else if(s.compare(string("false"))== 0){ 
    pNode = new Node(NODE_BOOL); 
    pNode->value = 0; 
    return pNode; 
  }  
  else if(s.compare(string("true"))== 0){ 
    pNode = new Node(NODE_BOOL); 
    pNode->value = 1; 
    return pNode; 
  }
  else if(s.compare(string("nil"))== 0){ 
    pNode = new Node(NODE_NIL); 
    pNode->value = 0; 
    return pNode; 
  }
  else { 
    pNode = new Node(NODE_SYMBOL); 
    pNode->symbol = s; 
    return pNode; 
  } 
}

Node* parenthesize(vector<string>* tokens, Node* pList){ 
  if(tokens->size() == 0){ 
    Node* pNode = pList->list.back(); 
    pList->list.pop_back();  
    return pNode; 
  }
  
  string token = tokens->at(0); 
  tokens->erase(tokens->begin()); 
  
  if(token.compare(string("(")) == 0){ 
    pList->list.push_back(parenthesize(tokens, new Node(NODE_LIST))); 
    return parenthesize(tokens, pList); 
  }
  else if(token.compare(string(")")) == 0){
    return pList; 
  }
  else { 
    pList->list.push_back(categorize(token)); 
    return parenthesize(tokens, pList);
  }
}

Node* READ(string s){ 

  s = regex_replace(s, regex("(\\()"), " ( ");
  s = regex_replace(s, regex("(\\))"), " ) ");
  s = regex_replace(s, regex("(\\r)"), " ");

  std::vector<std::string> tokenized = split(s, ' ');
  tokenized.pop_back(); //hack - apparently split has an extra blank on the end!

  if(tokenized.size() == 0){ 
    throw string("Empty!"); 
  }

  return parenthesize(&tokenized, new Node(NODE_LIST));  
}

//=====================================================================================
//       EVAL   
//=====================================================================================

//------------------------------------------------------------------------------
//  Env
//------------------------------------------------------------------------------

class Env { 
public: 
  Env(Env* _outer){ 
    outer = _outer; 
  }  

  Env(Env* _outer, vector<string>* binds, vector<Node*>* exprs){ 
    for(unsigned int i = 0; i < binds->size(); i++){ 
      set(binds->at(i), exprs->at(i)); 
    } 
 
    outer = _outer; 
  }  
  
  void set(string key, Node* value){ 
    data[key] = value; 
  }
  
  Env* find(string key){ 
    if(data.find(key) != data.end()){ 
      return this; 
    }else{ 
      if(outer == NULL){ 
        return NULL; 
      }
      return outer->find(key); 
    }
  }
  
  Node* get(string key){ 
    Env* pEnv = find(key); 
    if(pEnv == NULL){ 
      throw string("Not Found!"); 
    }
    return pEnv->data[key]; 
  }
  
  unordered_map<string, Node*> data; 
  
  Env* outer; 
}; 

Env repl_env(NULL); 

//forward:
Node* EVAL(Node* pNode, Env* pEnv); 

class AnonymousFunction : public Node { 

public: 
  AnonymousFunction(Env* _env, vector<string>* _binds, Node* _body ): Node(NODE_FUNCTION)
  { 
    env = _env; 
    binds = _binds; 
    body = _body; 
  }   

  Env* env; 
  vector<string>* binds; 
  Node* body;  

  virtual Node* call(vector<Node*>* args){
    Env* pNewEnv = new Env(env, binds, args); 
    return EVAL(body, pNewEnv); 
  
  //  Node* pNode = new Node(NODE_NUMBER); 
  //  pNode->value = args->at(0)->value - args->at(1)->value; 
  //  return pNode;   
  }
};


Node* eval_ast(Node* ast, Env* pEnv){ 
  if(ast->type == NODE_SYMBOL){ 
    //if(pEnv->contains(ast->symbol)){ 
      return pEnv->get(ast->symbol);
    //} else { 
    //  throw "cannot find symbol";
    //}
  } else if(ast->type == NODE_LIST){ 
    Node* pRet = new Node(NODE_LIST); 
    for(unsigned int i = 0; i < ast->list.size(); i++){ 
      pRet->list.push_back(EVAL(ast->list[i], pEnv));
    }
    return pRet;   
  } else { 
    return ast; 
  }
}

Node* EVAL(Node* pNode, Env* pEnv){

  if(pNode->type == NODE_LIST){ 
    if(pNode->list.size() == 0){ 
      return pNode; 
    } else {
      //specials: 
      if(pNode->list[0]->isSpecial(string("def!"))){ 
        string symbol = pNode->list[1]->symbol; 
        Node* value = EVAL(pNode->list[2], pEnv); 
        pEnv->set(symbol, value);
        return value;   
      }
      else if(pNode->list[0]->isSpecial(string("if"))){
        Node* test = EVAL(pNode->list[1], pEnv);
        if(test->isTrue()){ 
          return EVAL(pNode->list[2], pEnv);   
        }else{
          if(pNode->list.size() == 3){ 
            return new Node(NODE_NIL); 
          }
          else{ 
            return EVAL(pNode->list[3], pEnv); 
          }
        }
      }else if(pNode->list[0]->isSpecial(string("fn*"))){
        Node* argsList = pNode->list[1]; 
        Node* body = pNode->list[2];  
        vector<string>* pBinds = new vector<string>(); 
        for(unsigned int i = 0; i < argsList->list.size(); i++){ 
          pBinds->push_back(argsList->list.at(i)->symbol); 
        }
        return( new AnonymousFunction(pEnv, pBinds, body));  
      } else { 
        Node* pEvaluated = eval_ast(pNode, pEnv); 
        vector<Node*> args;
        for(unsigned int i = 1; i < pEvaluated->list.size(); i++){ 
          args.push_back(pEvaluated->list[i]); 
        }
        return pEvaluated->list[0]->call(&args);
      }  
    }
  } else { 
    return eval_ast(pNode, pEnv); 
  }

  return pNode; 
}

std::string PRINT(Node* pNode){ 
  return pNode->toString(); 
}

std::string rep(std::string str){ 
  //TODO: Add try catch around this:
  try{ 
    return PRINT(EVAL(READ(str), &repl_env));
  }catch(string e){ 
    return e;
  }catch(const std::runtime_error& re){
    return re.what();
  }catch(const std::exception& ex){
    return ex.what();
  }catch(...){
    return string("Unknown Error!");  
  }  
}

void mal_key_pressed(char ch){ 
//  print_string(to_string((int)ch)); 

  mal_print(ch); 

  if(ch == 8){ 
    if(input.length() > 0)
      input.pop_back(); 
    return; 
  }

  input.push_back(ch); 
  if(is_balanced(input) && input[input.length()-1] == '\r'){ 
    std::string output = rep(input);
    input.clear();  
    print_string(output);
    print_string(std::string("\r > "));
  }  
}

void mal_init(){ 
  std::string s("*** Welcome to Lisp ***\r > "); 
  print_string(s); 

//  string s2("(((ppp)))"); 
//  s2 = regex_replace(s2, regex("(\\()"), " ( ");
//  print_string(s2);

  repl_env.set("+", new AddFunction()); 
  repl_env.set("-", new SubtractFunction()); 
  repl_env.set("*", new MultiplyFunction()); 
  repl_env.set("=", new EqualsFunction()); 
  
}




