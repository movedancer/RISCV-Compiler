#include"front/lexical.h"

#include<map>
#include<cassert>
#include<string>

#define TODO assert(0 && "todo")

// #define DEBUG_DFA
// #define DEBUG_SCANNER

//辅助函数如下
frontend::TokenType get_op_type(std::string s){     // 获取运算符类型
    if (s == "+")   return frontend::TokenType::PLUS;
    else if (s == "-")  return frontend::TokenType::MINU;
    else if (s == "*")  return frontend::TokenType::MULT;
    else if (s == "/")  return frontend::TokenType::DIV;
    else if (s == "%")  return frontend::TokenType::MOD;
    else if (s == "<")  return frontend::TokenType::LSS;
    else if (s == ">")  return frontend::TokenType::GTR;
    else if (s == ":")  return frontend::TokenType::COLON;
    else if (s == "=")  return frontend::TokenType::ASSIGN;
    else if (s == ";")  return frontend::TokenType::SEMICN;
    else if (s == ",")  return frontend::TokenType::COMMA;
    else if (s == "(")  return frontend::TokenType::LPARENT;
    else if (s == ")")  return frontend::TokenType::RPARENT;
    else if (s == "[")  return frontend::TokenType::LBRACK;
    else if (s == "]")  return frontend::TokenType::RBRACK;
    else if (s == "{")  return frontend::TokenType::LBRACE;
    else if (s == "}")  return frontend::TokenType::RBRACE;
    else if (s == "!")  return frontend::TokenType::NOT;
    else if (s == "<=") return frontend::TokenType::LEQ;
    else if (s == ">=") return frontend::TokenType::GEQ;
    else if (s == "==") return frontend::TokenType::EQL;
    else if (s == "!=") return frontend::TokenType::NEQ;
    else if (s == "&&") return frontend::TokenType::AND;
    else if (s == "||") return frontend::TokenType::OR;
    else std::cout << "invalid Op:" << s << std::endl;
}

frontend::TokenType get_ident_type(std::string s){  // 获取关键字类型
    // TokenType中IDENFR、INTLTR、FLOATLTR都不是确定字符串，因此无法都在函数内判断
    if (s == "const")   return frontend::TokenType::CONSTTK;
    else if (s == "void")   return frontend::TokenType::VOIDTK;
    else if (s == "int")    return frontend::TokenType::INTTK;
    else if (s == "float")  return frontend::TokenType::FLOATTK;
    else if (s == "if")     return frontend::TokenType::IFTK;
    else if (s == "else")   return frontend::TokenType::ELSETK;
    else if (s == "while")  return frontend::TokenType::WHILETK;
    else if (s == "continue")   return frontend::TokenType::CONTINUETK;
    else if (s == "break")  return frontend::TokenType::BREAKTK;
    else if (s == "return") return frontend::TokenType::RETURNTK;
    else return frontend::TokenType::IDENFR;
}

bool isCharOp(char c){
    if(c=='+'||c=='-'||c=='*'||c=='/'||c=='%'||c=='<'||c=='>'||c=='='||c==':'||c==';'
        ||c=='('||c==')'||c=='['||c==']'||c=='{'||c=='}'||c=='!'||c=='&'||c=='|'||c==','){
        return true;
    }else{
        return false;
    }
}

bool isStringOp(std::string s){
    if(s=="+"||s=="-"||s=="*"||s=="/"||s=="%"||s=="<"||s==">"||s=="="||s==":"||s==";"
        ||s=="("||s==")"||s=="["||s=="]"||s=="{"||s=="}"||s=="!"||s=="&"||s=="|"||s==","
        ||s=="<="||s==">="||s=="=="||s=="!="||s=="&&"||s=="||"){
        return true;
    }else{
        return false;
    }
}

std::string frontend::toString(State s) {
    switch (s) {
    case State::Empty: return "Empty";
    case State::Ident: return "Ident";
    case State::IntLiteral: return "IntLiteral";
    case State::FloatLiteral: return "FloatLiteral";
    case State::op: return "op";
    default:
        assert(0 && "invalid State");
    }
    return "";
}

std::set<std::string> frontend::keywords= {
    "const", "int", "float", "if", "else", "while", "continue", "break", "return", "void"
};

std::string preScanner(std::ifstream &fin){
    std::string result;  // 去除注释的结果
    std::string read_line;    // 读取行代码
    bool in_multi_comment = false;  // 是否在多行注释内
    while(std::getline(fin, read_line)){
        size_t pos = 0;
        if(in_multi_comment){
            pos = read_line.find("*/");
            if(pos != std::string::npos){  //找到*/
                read_line.erase(0, pos + 2);
                in_multi_comment = false;
            }
            else{
                continue;  //没找到，继续下一行
            }
        }
        while(pos != std::string::npos){
            //不在多行注释中，基于查找起始注释
            size_t pos_s = read_line.find("//", pos);
            size_t pos_m = read_line.find("/*", pos);
            if(pos_s == pos){
                read_line.erase(pos);  // 当前位置就是//注释，直接移除行
                break;
            }
            else if(pos_m ==pos){
                size_t pos_m_end = read_line.find("*/", pos_m + 2);  // 在起始注释后查找结束注释
                if(pos_m_end != std::string::npos){  // 多行注释只有一行
                    read_line.erase(pos_m, pos_m_end - pos_m + 2);
                }
                else{
                    read_line.erase(pos_m);
                    in_multi_comment = true;   // 当前位于多行注释内
                }
                break;  // 仅查找多行注释，优先级最高;
            }
            pos = std::min(pos_s, pos_m);  // 取最靠前位置，防止错过
        }
        result += read_line + "\n";
    }
    return result;
}

frontend::DFA::DFA(): cur_state(frontend::State::Empty), cur_str() {}

frontend::DFA::~DFA() {}

bool frontend::DFA::next(char input, Token& buf) {
#ifdef DEBUG_DFA
#include<iostream>
    std::cout << "in state [" << toString(cur_state) << "], input = \'" << input << "\', str = " << cur_str << "\t";
#endif
    //TODO;
    bool flag = false;      // 不直接返回true/false，否则结尾的"next state"无法输出
    switch (cur_state){
        case frontend::State::Empty:    // State刚被reset过
            if (isspace(input)){                    // 维持Empty状态
                reset();
                break;
            }
            else if (isalpha(input) || input=='_'){ // 转换到Ident状态(a~z、A~Z、_)
                cur_state = frontend::State::Ident;
                cur_str += input;
                break;
            }
            else if (isdigit(input)){               // 转换到IntLiteral状态
                cur_state = frontend::State::IntLiteral;
                cur_str += input;
                break;
            }
            else if (input == '.'){                 // 转换到FloatLiteral状态
                cur_state = frontend::State::FloatLiteral;
                cur_str += input;
                break;
            }
            else if (isCharOp(input)){              // 转换到op状态
                cur_state = frontend::State::op;
                cur_str += input;
                break;
            }
            else{                                   // 状态转换非法
                assert(0 && "invalid next State");
                break;
            }
        case frontend::State::Ident:
            if (isspace(input)){                    // 转换到Empty状态，此时字符串读取完毕，输出
                buf.type = get_ident_type(cur_str);
                buf.value = cur_str;
                reset();                            // 调用reset重置状态
                flag = true;
                break;
            }
            else if (isalpha(input) || isdigit(input) || input == '_'){ // 维持Ident状态，继续读取字符串，直到出现状态转移
                cur_str += input;
                break;
            }
            else if (isCharOp(input)){              // 转换到Op状态，同理Empty
                buf.type = get_ident_type(cur_str);
                buf.value = cur_str;                // 将此前字符串同步至buf以供输出
                cur_state = frontend::State::op;
                cur_str = input;
                flag = true;
                break;
            }
            else{                                   // 其余转换均非法
                assert(0 && "invalid next State");
                break;
            }
        case frontend::State::op:
            if (isspace(input)){                    // 转换到Empty状态
                buf.type = get_op_type(cur_str);
                buf.value = cur_str;
                reset();
                flag = true;
                break;
            }
            else if (isalpha(input) || input=='_'){ // 转换到Ident状态
                buf.type = get_op_type(cur_str);
                buf.value = cur_str;
                cur_state = frontend::State::Ident;
                cur_str = input;
                flag = true;
                break;
            }
            else if (isdigit(input)){               // 转换到IntLiteral状态
                buf.type = get_op_type(cur_str);
                buf.value = cur_str;
                cur_state = frontend::State::IntLiteral;
                cur_str = input;
                flag = true;
                break;
            }
            else if (input == '.'){                 // 转换到FloatLiteral状态
                buf.type = get_op_type(cur_str);
                buf.value = cur_str;
                cur_state = frontend::State::FloatLiteral;
                cur_str = input;
                flag = true;
                break;
            }
            else if (isCharOp(input)){              // 维持Op状态
                if (isStringOp(cur_str + input)){
                    cur_str += input;
                    break;
                }
                else{       // 已经获得完整操作符
                    buf.type = get_op_type(cur_str);
                    buf.value = cur_str;
                    cur_state = frontend::State::op;
                    cur_str = input;
                    flag = true;
                    break;
                }
            }
            else{                                   // 其余转换均非法
                assert(0 && "invalid next State");
                break;
            }
        case frontend::State::IntLiteral:
            if (isspace(input)){                    // 转换到Empty状态
                buf.type = frontend::TokenType::INTLTR;
                buf.value = cur_str;
                reset();
                flag = true;
                break;
            }
            else if (isdigit(input) || (input >= 'a' && input <= 'f') || (input >= 'A' && input <= 'F') 
                        || input == 'x' || input == 'X'){   // 维持IntLiteral状态
                cur_str += input;
                break;
            }
            else if (input == '.'){                 // 转换到FloatLiteral状态
                cur_state = frontend::State::FloatLiteral;
                cur_str += input;
                break;
            }
            else if (isCharOp(input)){              // 转换到Op状态
                buf.type = frontend::TokenType::INTLTR;
                buf.value = cur_str;
                cur_state = frontend::State::op;
                cur_str = input;
                flag = true;
                break;
            }
            else{                                   // 其余转换均非法
                assert(0 && "invalid next State");
                break;
            }
        case frontend::State::FloatLiteral:
            if (isspace(input)){                    // 转换到Empty状态
                buf.type = frontend::TokenType::FLOATLTR;
                buf.value = cur_str;
                reset();
                flag = true;
                break;
            }
            else if (isdigit(input)){               // 转换到FloatLiteral状态
                cur_str += input;
                break;
            }            
            else if (isCharOp(input)){              // 转换到Op状态
                buf.type = frontend::TokenType::FLOATLTR;
                buf.value = cur_str;
                cur_state = frontend::State::op;
                cur_str = input;
                flag = true;
                break;
            }
            else{                                   // 其余转换均非法
                assert(0 && "invalid next State");
                break;
            }
        default: assert(0 && "invalid State");
    }

    return flag;

    // 打印下一个状态
#ifdef DEBUG_DFA
    std::cout << "next state is [" << toString(cur_state) << "], next str = " << cur_str << "\t, ret = " << ret << std::endl;
#endif
}

void frontend::DFA::reset() {
    cur_state = State::Empty;
    cur_str = "";
}

frontend::Scanner::Scanner(std::string filename): fin(filename) {
    if(!fin.is_open()) {
        assert(0 && "in Scanner constructor, input file cannot open");
    }
}

frontend::Scanner::~Scanner() {
    fin.close();
}

std::vector<frontend::Token> frontend::Scanner::run() {
    //TODO;
    std::vector<Token> result;
    Token tk;
    DFA dfa;
    std::string s = preScanner(fin);    // 删除注释
    s += "\n";                          // s结尾加上换行符，用于状态机末尾确认
    for (auto c : s){
        if (dfa.next(c, tk)){
            // 压入token
            result.push_back(tk);
            // 打印结果
            #ifdef DEBUG_SCANNER
            #include <iostream>
                std::cout << "token: " << toString(tk.type) << "\t" << tk.value << std::endl;
            #endif
        }
    }
    return result;
}