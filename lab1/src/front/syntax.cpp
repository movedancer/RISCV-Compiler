#include"front/syntax.h"

#include<iostream>
#include<cassert>

using frontend::Parser;

// #define DEBUG_PARSER
#define TODO assert(0 && "todo")
#define CUR_TOKEN_IS(tk_type) (token_stream[index].type == TokenType::tk_type)
#define PARSE_TOKEN(tk_type) root->children.push_back(parseTerm(root, TokenType::tk_type))
#define PARSE(name, type) auto name = new type(root); assert(parse##type(name)); root->children.push_back(name); 

/*消除了左递归的文法规则：
CompUnit -> (Decl | FuncDef) [CompUnit]
Decl -> ConstDecl | VarDecl
ConstDecl -> 'const' BType ConstDef { ',' ConstDef } ';'
BType -> 'int' | 'float'
ConstDef -> Ident { '[' ConstExp ']' } '=' ConstInitVal
ConstInitVal -> ConstExp | '{' [ ConstInitVal { ',' ConstInitVal } ] '}'
VarDecl -> BType VarDef { ',' VarDef } ';'
VarDef -> Ident { '[' ConstExp ']' } [ '=' InitVal ]
InitVal -> Exp | '{' [ InitVal { ',' InitVal } ] '}'
FuncDef -> FuncType Ident '(' [FuncFParams] ')' Block
FuncType -> 'void' | 'int' | 'float'
FuncFParam -> BType Ident ['[' ']' { '[' Exp ']' }]
FuncFParams -> FuncFParam { ',' FuncFParam }
Block -> '{' { BlockItem } '}'
BlockItem -> Decl | Stmt
Stmt -> LVal '=' Exp ';' | Block | 'if' '(' Cond ')' Stmt [ 'else' Stmt ] | 'while' '(' Cond ')' Stmt | 'break' ';' | 'continue' ';' | 'return' [Exp] ';' | [Exp] ';'
Exp -> AddExp
Cond -> LOrExp
LVal -> Ident {'[' Exp ']'}
Number -> IntConst | floatConst
PrimaryExp -> '(' Exp ')' | LVal | Number
UnaryExp -> PrimaryExp | Ident '(' [FuncRParams] ')' | UnaryOp UnaryExp
UnaryOp -> '+' | '-' | '!'
FuncRParams -> Exp { ',' Exp }
MulExp -> UnaryExp { ('*' | '/' | '%') UnaryExp }
AddExp -> MulExp { ('+' | '-') MulExp }
RelExp -> AddExp { ('<' | '>' | '<=' | '>=') AddExp }
EqExp -> RelExp { ('==' | '!=') RelExp }
LAndExp -> EqExp [ '&&' LAndExp ]
LOrExp -> LAndExp [ '||' LOrExp ]
ConstExp -> AddExp
*/

Parser::Parser(const std::vector<frontend::Token>& tokens): index(0), token_stream(tokens) {}

Parser::~Parser() {}

frontend::CompUnit* Parser::get_abstract_syntax_tree(){
    //TODO;
    return parseCompUnit(nullptr);
}

void Parser::log(AstNode* node){
#ifdef DEBUG_PARSER
        std::cout << "in parse" << toString(node->type) << ", cur_token_type::" << toString(token_stream[index].type) << ", token_val::" << token_stream[index].value << '\n';
#endif
}

// 辅助函数，用于判断某文法的构造
// CompUnit -> (Decl | FuncDef) [CompUnit]
frontend::CompUnit *Parser::parseCompUnit(AstNode *parent){ // 构造的是根节点，因此返回值不是bool型
    CompUnit *result = new CompUnit(parent);             // 构造根节点, 修改了构造逻辑，创建的Decl *result会被设置为parent的children
    log(result);                                         // 记录检查点
    bool decl = false;                                   // Decl匹配
    bool funcDef = false;                                // FuncDef匹配
    int last_index = index;                              // 分支匹配时，记录当前index回溯
    int last_child_nums = result->children.size();       // 记录当前孩子数
    if(Match_Decl().count(token_stream[index].type)){    // 匹配到Decl
        decl = parseDecl(result);                        // 进入分支进行匹配
        if(!decl){
            retreat(last_index, result, last_child_nums);// 不匹配，回溯
        }
    }
    if(!decl && Match_FuncDef().count(token_stream[index].type)){
        funcDef = parseFuncDef(result);                  // 进入分支进行匹配
        if(!funcDef){
            assert(0 && "error in parseCompUnit");       // 不存在两个必须可选的都不匹配，报错
        }
    }
    if(Match_Compunit().count(token_stream[index].type)){
        parseCompUnit(result);                           // 递归调用可选分支
    }
    return result;
}

// Decl -> ConstDecl | VarDecl
bool Parser::parseDecl(AstNode *parent){
    Decl *result = new Decl(parent);                     // 构造根节点
    log(result);                                         // 记录检查点
    bool constDecl = false;                              // 分支ConstDecl匹配标志
    bool varDecl = false;                                // 分支VarDecl匹配标志
    int last_index = index;                              // 记录本次索引，用于回溯
    int last_children_nums = result->children.size();    // 记录本次孩子数，用于回溯
    if(Match_ConstDecl().count(token_stream[index].type)){  // 统计token流，匹配到ConstDecl
        constDecl = parseConstDecl(result);              // 语法匹配，递归下降
        if(!constDecl){                                  // 无匹配，回溯
            retreat(last_index, result, last_children_nums);
        }
    }
    if(!constDecl && Match_VarDecl().count(token_stream[index].type)){  // 统计token流，匹配到VarDecl
        varDecl = parseVarDecl(result);                  // 语法匹配，递归下降
        if(!varDecl){                                    // 全都不匹配，当前分支错误
            return false;                                // Decl匹配错误，回溯，上一层切换分支
        }
    }
    return true;                                         // 有匹配，Decl分支匹配成功，递归成功
}

// ConstDecl -> 'const' BType ConstDef { ',' ConstDef } ';'
bool Parser::parseConstDecl(AstNode *parent){
    ConstDecl *result = new ConstDecl(parent);       // 构造根节点
    log(result);                                     // 记录检查点
    // 当前语法没有分支，直接使用first集匹配
    if(token_stream[index].type != frontend::TokenType::CONSTTK){
        return false;                                // first集不匹配，当前分支错误，回溯上一层，重新选择分支
    }
    new Term(token_stream[index], result);           // 匹配到终结符，加入语法树，并设置父节点为result
    ++index;                                         // 完成当前匹配，index++下一个
    if(!parseBType(result)){                         // 匹配BType失败
        return false;
    }
    if(!parseConstDef(result)){                      // 匹配ConstDef失败
        return false;
    }
    //"{...}"表示0~n次匹配, 开始循环
    while(token_stream[index].type == frontend::TokenType::COMMA){ // 匹配到","表示进入循环
        new Term(token_stream[index], result);       // 匹配到终结符，加入语法树，并设置父节点为result
        ++index;                                     // 完成当前匹配，index++下一个
        if(!parseConstDef(result)){
            return false;                            // 进入循环但是没有匹配ConstDef，说明分支错误，回溯上一层，表示Constdecl分支错误
        }
    }
    if(token_stream[index].type != frontend::TokenType::SEMICN){
        return false;                                // 没有匹配到分号，说明当前分支出错，回溯上一层，ConstDecl分支错误
    }
    new Term(token_stream[index], result);           // 匹配到";"终结符，加入语法树，设置父节点为result
    ++index;                                         // 完成当前匹配，index++下一个
    return true;                                     // 当前分支匹配成功
}

// BType -> 'int' | 'float'
bool Parser::parseBType(AstNode *parent){
    BType *result = new BType(parent);               // 创建根节点
    log(result);                                     // 保存检查点
    //没有分支，全是终结符
    if(token_stream[index].type == frontend::TokenType::INTTK){
        new Term(token_stream[index], result);       // 匹配到int终结符，加入语法树，设置父节点为result
        ++index;                                     // index++, 获得下一个待匹配token
    }
    else if(token_stream[index].type == frontend::TokenType::FLOATTK){
        new Term(token_stream[index], result);       // 匹配到float终结符，加入语法树，设置父节点为result
        ++index;                                     // index++, 获得下一个待匹配token
    }
    else{
        return false;                                // 全部匹配失败，回溯上一层，BType分支错误
    }
    return true;                                     // 匹配成功
}

// ConstDef -> Ident { '[' ConstExp ']' } '=' ConstInitVal
bool Parser::parseConstDef(AstNode *parent){
    ConstDef *result = new ConstDef(parent);           // 构造根节点
    log(result);                                       // 保存检查点
    // 当前没有分支，为终结符匹配
    if(token_stream[index].type != frontend::TokenType::IDENFR){
        return false;                                  // 没有匹配到终结符，当前分支错误，回溯
    }
    new Term(token_stream[index], result);             // 匹配到终结符，加入语法树，设置父节点为result
    ++index;                                           // ++index, 匹配下一个字符
    //出现"{...}"，0~n次匹配，需要进入循环
    while(token_stream[index].type == frontend::TokenType::LBRACK){
        new Term(token_stream[index], result);         // 匹配到"["，进入循环，将终结符加入语法树，设置父节点为result
        ++index;
        if(!parseConstExp(result)){                    // 未匹配到ConstExp，当前分支错误，回溯，ConstDef分支错误。
            return false;
        }
        if(token_stream[index].type != frontend::TokenType::RBRACE){ // 匹配"]"，未匹配到，当前分支错误，回溯
            return false;
        }
        new Term(token_stream[index], result);         // 匹配到"]"，将终结符加入语法树，设置父节点为result
        ++index;
    }
    if(token_stream[index].type != frontend::TokenType::ASSIGN){
        return false;                                  // 未匹配到"=", 分支错误，回溯
    }
    new Term(token_stream[index], result);             // 匹配到终结符"="，加入语法树
    ++index;
    if(!parseConstInitVal(result)){                    
        return false;                                  // 结尾未匹配，语法错误，回溯
    }
    return true;
}

// ConstInitVal -> ConstExp | '{' [ ConstInitVal { ',' ConstInitVal } ] '}'
bool Parser::parseConstInitVal(AstNode *parent){
    ConstInitVal *result = new ConstInitVal(parent);        // 创建根节点
    log(result);
    bool constexp = false;                                  // 设置分支1
    bool lbrace = false;                                    // 设置分支2
    int last_index = index;                                 // 保存当前索引
    int last_children_nums = result->children.size();       // 保存当前孩子数
    if(Match_ConstExp().count(token_stream[index].type)){   // 分支1匹配
        constexp = parseConstExp(result);
        if(!constexp){
            retreat(last_index, result, last_children_nums);// 匹配失败，回溯
        }
    }
    if(!constexp && token_stream[index].type == frontend::TokenType::LBRACE){ // 分支2匹配
        new Term(token_stream[index], result);              // 匹配终结符，加入语法树
        ++index;
        // 出现"[...]", 表示0~1次出现，直接判断，不需要循环
        if(Match_ConstInitVal().count(token_stream[index].type)){
            if(!parseConstInitVal(result)){
                return false;                               // 进入可选分支后匹配出错，回溯
            }
            // 出现"{...}", 表示0~n次匹配，循环
            while(token_stream[index].type == frontend::TokenType::COMMA){
                new Term(token_stream[index], result);      // 匹配终结符，加入语法树
                ++index;
                if(!parseConstInitVal(result)){
                    return false;
                }
            }
        }
        if(token_stream[index].type == frontend::TokenType::RBRACE){
            return false;
        }
        new Term(token_stream[index], result);
        ++index;
        lbrace = true;                                      // 分支2完成匹配，设置分支为true
    }
    if(!lbrace && !constexp){
        return false;                                       // 分支1和分支2全部错误，ConstInitVal分支错误，回溯
    }
    return true;
}

// VarDecl -> BType VarDef { ',' VarDef } ';'
bool Parser::parseVarDecl(AstNode *parent){
    VarDecl *result = new VarDecl(parent);                 // 创建根节点
    log(result);                                           // 设置检查点
    // 但当前语法不存在分支，直接匹配
    if(!parseBType(result)){                              // 匹配出错，回溯
        return false;
    }
    if(!parseVarDef(result)){                             // 匹配出错，回溯
        return false;
    }
    // 出现"{...}"，0~n次匹配，使用循环
    while(token_stream[index].type == frontend::TokenType::COMMA){ // 进入匹配
        new Term(token_stream[index], result);             // 匹配终结符，加入语法树
        ++index;
        if(!parseVarDef(result)){                         // 格式出错，当前语法分支出错，回溯
            return false;
        }
    }
    if(token_stream[index].type != frontend::TokenType::SEMICN){   // 匹配结尾终结符
        return false;                                      // 格式出错，回溯
    }
    new Term(token_stream[index], result);               // 匹配成功，加入语法树
    ++index;
    return true;
}

// VarDef -> Ident { '[' ConstExp ']' } [ '=' InitVal ]
bool Parser::parseVarDef(AstNode *parent){
    VarDef *result = new VarDef(parent);                   // 创建根节点
    log(result);                                           // 记录检查点
    // 没有分支，直接匹配
    if(token_stream[index].type != frontend::TokenType::IDENFR){    // 匹配终结符
        return false;
    }
    new Term(token_stream[index], result);                 // 匹配终结符成功，加入语法树
    ++index;
    // 出现"{...}"，0~n次匹配，使用循环
    while(token_stream[index].type == frontend::TokenType::LBRACK){ // 匹配终结符
        new Term(token_stream[index], result);             // 匹配终结符成功，加入语法树
        ++index;
        if(!parseConstExp(result)){                        // 进入ConstExp规则，出错，则当前分支错误，回溯
            return false;
        }
        if(token_stream[index].type != frontend::TokenType::RBRACK){ // 匹配终结符
            return false;
        }
        new Term(token_stream[index], result);             // 匹配终结符成功，加入语法树
        ++index;
    }
    // 进入可选分支"[...]"，出现0~1次，直接匹配
    if(token_stream[index].type == frontend::TokenType::ASSIGN){
        new Term(token_stream[index], result);             // 匹配进入可选分支，加入终结符至语法树
        ++index;
        if(!parseInitVal(result)){                        // 可选分支规则错误，回溯
            return false;
        }
    }
    return true;
}

// InitVal -> Exp | '{' [ InitVal { ',' InitVal } ] '}'
bool Parser::parseInitVal(AstNode *parent){
    InitVal *result = new InitVal(parent);                 // 创建根节点
    log(result);                                           // 设置检查点
    // 有分支，进行判断
    bool exp = false;
    bool lbrace = false;
    int last_index = index;
    int last_child_nums = result->children.size();
    if(Match_Exp().count(token_stream[index].type)){       // 进入分支Exp，进行匹配
        exp = parseExp(result);
        if(!exp){                                          // Exp匹配失败，回溯
            retreat(last_index, result, last_child_nums);
        }
    }
    if(!exp && token_stream[index].type == frontend::TokenType::LBRACE){  // 进入分支'{' [ InitVal { ',' InitVal } ] '}'进行匹配
        new Term(token_stream[index], result);             // 匹配到终结符，加入语法树父节点
        ++index;
        // 进入可选分支"[...]"，出现0~1次，直接匹配
        if(Match_InitVal().count(token_stream[index].type)){
            if(!parseInitVal(result)){                    // 进入匹配，出现规则错误，回溯
                return false;
            }
            // 出现"{...}"，0~n次匹配，使用循环
            while(token_stream[index].type == frontend::TokenType::COMMA){
                new Term(token_stream[index], result);    // 匹配到终结符，加入语法树
                ++index;
                if(!parseInitVal(result)){                // InitVal规则出错，回溯
                    return false;
                }
            }
        }
        if(token_stream[index].type != frontend::TokenType::RBRACE){
            return false;                                  // 终结符匹配失败，语法错误，回溯
        }
        new Term(token_stream[index], result);             // 匹配终结符，加入语法树
        ++index;
        lbrace = true;                                // 全部匹配完成，设置当前分支匹配成功
    }
    if(!lbrace && !exp){
        return false;
    }
    return true;
}

// FuncDef -> FuncType Ident '(' [FuncFParams] ')' Block
bool Parser::parseFuncDef(AstNode *parent){
    FuncDef *result = new FuncDef(parent);                  // 创建根节点
    log(result);                                            // 设置检查点
    // 当前语法没有分支，直接匹配
    if(!parseFuncType(result)){
        return false;                                       // functype匹配失败，回溯
    }
    if(token_stream[index].type != frontend::TokenType::IDENFR){
        return false;                                        // 终结符匹配失败。回溯
    }
    new Term(token_stream[index], result);                   // 匹配到终结符，加入语法树
    ++index;
    if(token_stream[index].type != frontend::TokenType::LPARENT){
        return false;                                        // 终结符匹配失败。回溯
    }
    new Term(token_stream[index], result);                 // 匹配到终结符，加入语法树
    ++index;
    // 进入可选分支"[...]"，出现0~1次，直接匹配
    if(Match_FuncFParams().count(token_stream[index].type)){
        if(!parseFuncFParams(result)){
            return false;                                    // 分支匹配出错，回溯
        }
    }
    if(token_stream[index].type != frontend::TokenType::RPARENT){
        return false;                                        // 终结符匹配出错，回溯
    }
    new Term(token_stream[index], result);                   // 匹配到终结符，加入语法树
    ++index;
    if(!parseBlock(result)){
        return false;                                        // block匹配出错，回溯
    }
    return true;
}

// FuncType -> 'void' | 'int' | 'float'
bool Parser::parseFuncType(AstNode *parent){
    FuncType *result = new FuncType(parent);                  // 创建根节点
    log(result);                                              // 设置检查点
    //无分支，直接匹配终结符
    if(token_stream[index].type == frontend::TokenType::VOIDTK){
        new Term(token_stream[index], result);                // 匹配到终结符，加入语法树
        ++index;
    }
    else if(token_stream[index].type == frontend::TokenType::INTTK){
        new Term(token_stream[index], result);                // 匹配到终结符，加入语法树
        ++index;
    }
    else if(token_stream[index].type == frontend::TokenType::FLOATTK){
        new Term(token_stream[index], result);                // 匹配到终结符，加入语法树
        ++index;
    }
    else{
        return false;                                         // 匹配不到终结符，回溯，当前分支错误
    }
    return true;
}

// FuncFParam -> BType Ident ['[' ']' { '[' Exp ']' }]
bool Parser::parseFuncFParam(AstNode *parent){
    FuncFParam *result = new FuncFParam(parent);              // 创建根节点
    log(result);                                              // 设置检查点
    // 没有分支，直接匹配
    if(!parseBType(result)){
        return false;                                         // btype分支匹配错误，回溯
    }
    if(token_stream[index].type != frontend::TokenType::IDENFR){
        return false;                                         // 终结符匹配错误，回溯
    }
    new Term(token_stream[index], result);                    // 终结符匹配成功加入语法树
    ++index;
    // 进入可选分支"[...]"，出现0~1次，直接匹配
    if(token_stream[index].type == frontend::TokenType::LBRACK){
        new Term(token_stream[index], result);                // 终结符匹配成功加入语法树
        ++index;
        if(token_stream[index].type != frontend::TokenType::RBRACK){
            return false;                                     // 终结符匹配失败，回溯，当前分支错误
        }
        new Term(token_stream[index], result);                // 终结符匹配成功加入语法树
        ++index;
        // 进入多选分支"{...}", 0~n次匹配，while循环
        while(token_stream[index].type == frontend::TokenType::LBRACK){
            new Term(token_stream[index], result);            // 终结符匹配成功加入语法树
            ++index;
            if(!parseExp(result)){
                return false;                                 // exp分支错误，回溯
            }
            if(token_stream[index].type != frontend::TokenType::RBRACK){
                return false;                                 // 终结符匹配失败，回溯，当前分支错误
            }
            new Term(token_stream[index], result);            // 终结符匹配成功加入语法树
            ++index;
        }
    }
    return true;
}

// FuncFParams -> FuncFParam { ',' FuncFParam }
bool Parser::parseFuncFParams(AstNode *parent){
    FuncFParams *result = new FuncFParams(parent);             // 创建根节点
    log(result);                                               // 设置检查点
    // 无分支，直接匹配
    if(!parseFuncFParam(result)){
        return false;                                          // funcfparam分支匹配错误，回溯
    }
    // 出现"{...}"，0~n次匹配，使用循环
    while(token_stream[index].type == frontend::TokenType::COMMA){
        new Term(token_stream[index], result);                 // 进入{...}内，终结符匹配成功，加入语法树
        ++index;
        if(!parseFuncFParam(result)){
            return false;                                      // funcfparam分支匹配错误，回溯
        }
    }
    return true;
}

// Block -> '{' { BlockItem } '}'
bool Parser::parseBlock(AstNode *parent){
    Block *result = new Block(parent);
    log(result);
    // 无分支，直接匹配
    if(token_stream[index].type != frontend::TokenType::LBRACE){
        return false;                                           // 终结符匹配失败，回溯
    }
    new Term(token_stream[index], result);                      // 终结符匹配成功，加入语法树
    ++index;
    // 出现"{...}"，0~n次匹配，使用循环
    while(Match_BlockItem().count(token_stream[index].type)){
        bool blockitem_result = parseBlockItem(result);
        if(!blockitem_result){
            return false;                                       // blockitem匹配出错，回溯
        }
    }
    if(token_stream[index].type != frontend::TokenType::RBRACE){
        return false;                                           // 终结符匹配失败，回溯
    }
    new Term(token_stream[index], result);                    // 终结符匹配成功，加入语法树
    ++index;
    return true;
}

// BlockItem -> Decl | Stmt
bool Parser::parseBlockItem(AstNode *parent){
    BlockItem *result = new BlockItem(parent);
    log(result);
    // 存在分支，设置分支
    bool decl = false;                                           // 进入Decl分支
    bool stmt = false;                                           // 进入stmt分支
    int last_index = index;                                      // 保存当前索引，用于回溯
    int last_child_nums = result->children.size();               // 保存当前孩子数，用于回溯
    if(Match_Decl().count(token_stream[index].type)){
        decl = parseDecl(result);
        if(!decl){                                               // decl分支匹配错误，回溯
            retreat(last_index, result, last_child_nums);
        }
    }
    if(!decl && Match_Stmt().count(token_stream[index].type)){   // stmt匹配，递归下降匹配
        stmt = parseStmt(result);
        if(!stmt){
            return false;                                        // 两个分支全部错误，不可能，回溯
        }
    }
    return true;
}

// Stmt -> LVal '=' Exp ';' | Block | 'if' '(' Cond ')' Stmt [ 'else' Stmt ] | 'while' '(' Cond ')' Stmt | 'break' ';' | 'continue' ';' | 'return' [Exp] ';' | [Exp] ';'
bool Parser::parseStmt(AstNode *parent){
    Stmt *result = new Stmt(parent);
    log(result);
    // 当前存在较多分支，进行标注
    bool lval = false;                                            // lval分支
    bool block = false;                                           // block分支
    bool keyword = false;                                         // 匹配'if'、'while'、'break'、'continue'、'return'终结符关键词分支
    bool exp = false;                                             // 匹配exp分支
    bool semicn = false;                                          // 匹配";"分支
    int last_index = index;
    int last_child_nums = result->children.size();
    if(Match_LVal().count(token_stream[index].type)){             // 分支LVal '=' Exp ';'
        lval = parseLVal(result);                                 // 匹配LVal
        if(!lval){
            retreat(last_index, result, last_child_nums);         // 匹配错误，lval分支错误，下一分支匹配
            goto BlockCase;                                       // 进入下一分支
        }
        if(token_stream[index].type != frontend::TokenType::ASSIGN){   // 分支'='进行匹配
            lval = false;
            retreat(last_index, result, last_child_nums);         // 当前分支匹配错误，回溯
            goto BlockCase;                                       // 进入下一分支
        }
        new Term(token_stream[index], result);                    // 匹配到终结符，加入语法树
        ++index;
        if(!parseExp(result)){                                    // 进入Exp匹配
            lval = false;                                         // 提前置为false
            retreat(last_index, result, last_child_nums);
            goto BlockCase;                                       // 进入下一分支
        }
        if(token_stream[index].type != frontend::TokenType::SEMICN){   // 进入';'分支
            lval = false;
            retreat(last_index, result, last_child_nums);
            goto BlockCase;
        }
        new Term(token_stream[index], result);
        ++index;
        return true;
    }
    // block分支
    BlockCase:
    if(!lval && Match_Block().count(token_stream[index].type)){   // 进入分支Block
        block = parseBlock(result);
        if(!block){                                               //block分支出错，回溯
            retreat(last_index, result, last_child_nums);
        }
        else{
            return true;                                          // 否则当前匹配成功
        }
    }
    if(token_stream[index].type == frontend::TokenType::IFTK){    // 进入分支'if' '(' Cond ')' Stmt [ 'else' Stmt ]
        new Term(token_stream[index], result);                  // 匹配终结符'if'
        ++index;
        if(token_stream[index].type != frontend::TokenType::LPARENT){  // 匹配终结符'('
            keyword = false;
            retreat(last_index, result, last_child_nums);
            goto ExpCase;                                         // 前往下一分支
        }
        new Term(token_stream[index], result);
        ++index;
        if(!parseCond(result)){                                   // 进入分支Cond
            keyword = false;
            retreat(last_index, result, last_child_nums);
            goto ExpCase;                                         // 前往下一分支
        }
        if(token_stream[index].type != frontend::TokenType::RPARENT){  // ')'
            keyword = false;
            retreat(last_index, result, last_child_nums);
            goto ExpCase;
        }
        new Term(token_stream[index], result);
        ++index;
        if(!parseStmt(result)){                                    // 进入Stmt分支
            keyword = false;
            retreat(last_index, result, last_child_nums);
            goto ExpCase;
        }
        // 进入可选分支"[...]"，出现0~1次，直接匹配
        if(token_stream[index].type == frontend::TokenType::ELSETK){// 进入分支[ 'else' Stmt ]
            new Term(token_stream[index], result);                  // 匹配'else'
            ++index;
            if (!parseStmt(result)){                                // 匹配Stmt
                keyword = false;
                retreat(last_index, result, last_child_nums);
                goto ExpCase;
            }
        }
        return true;
    }
    else if(token_stream[index].type == frontend::TokenType::WHILETK){ // 进入分支'while' '(' Cond ')' Stmt
        new Term(token_stream[index], result);                         // 匹配终结符'while'
        ++index;
        if(token_stream[index].type != frontend::TokenType::LPARENT){  // 匹配'('
            keyword = false;
            retreat(last_index, result, last_child_nums);              // 匹配失败，回溯
            goto ExpCase;                                              // 进入下一分支
        }
        new Term(token_stream[index], result);                         // 匹配终结符，加入语法树
        ++index;
        if(!parseCond(result)){                                        // 进入分支Cond
            keyword = false;
            retreat(last_index, result, last_child_nums);
            goto ExpCase;
        }
        if(token_stream[index].type != frontend::TokenType::RPARENT){  // 进入分支')'
            keyword = false;
            retreat(last_index, result, last_child_nums);
            goto ExpCase;
        }
        new Term(token_stream[index], result);                         // 匹配终结符，加入语法树
        ++index;
        if(!parseStmt(result)){                                        // 进入分支Stmt
            keyword = false;
            retreat(last_index, result, last_child_nums);
            goto ExpCase;
        }
        return true;
    }
    else if(token_stream[index].type == frontend::TokenType::BREAKTK){  // 进入匹配'break' ';'
        new Term(token_stream[index], result);                          // 'break'终结符匹配成功，加入语法树
        ++index;
        if(token_stream[index].type != frontend::TokenType::SEMICN){    // 匹配';'
            keyword = false;
            retreat(last_index, result, last_child_nums);
            goto ExpCase;
        }
        new Term(token_stream[index], result);
        ++index;
        return true;
    }
    else if(token_stream[index].type == frontend::TokenType::CONTINUETK){// 进入分支'continue' ';'
        new Term(token_stream[index], result);                           // 进入匹配'continue'，加入语法树
        ++index;
        if(token_stream[index].type != frontend::TokenType::SEMICN){     // 进入匹配';'
            keyword = false;
            retreat(last_index, result, last_child_nums);
            goto ExpCase;
        }
        new Term(token_stream[index], result);
        ++index;
        return true;
    }
    else if(token_stream[index].type == frontend::TokenType::RETURNTK){  // 进入分支'return' [Exp] ';'
        new Term(token_stream[index], result);                           // 匹配'return'，加入语法树
        ++index;
        // 进入可选分支"[...]"，出现0~1次，直接匹配
        if(Match_Exp().count(token_stream[index].type)){                 // 匹配[Exp]
            parseExp(result);
        }
        if(token_stream[index].type != frontend::TokenType::SEMICN){     // 匹配';'
            keyword = false;
            retreat(last_index, result, last_child_nums);                // 匹配错误，回溯
            goto ExpCase;
        }
        new Term(token_stream[index], result);
        ++index;
        return true;
    }
    // 进入exp分支
    ExpCase:
    if(!lval && !block && !keyword && Match_Exp().count(token_stream[index].type)){  // 进入分支[Exp] ';'
        exp = parseExp(result);                                          // 进入Exp分支
        if(exp){
            if (token_stream[index].type == frontend::TokenType::SEMICN){
                new Term(token_stream[index], result);
                ++index;
                return true;
            }
            else{
                exp = false;
                retreat(last_index, result, last_child_nums);
                goto SEMICNCase;                                         // 匹配失败，进入下一分支
            }
        }
        else{
            exp = false;
            retreat(last_index, result, last_child_nums);
            goto SEMICNCase;
        }
    }
    // 由于[exp]可能为空，因此还需要匹配";"，最后一个分支
    SEMICNCase:
    if(!lval && !block && !keyword && !exp && token_stream[index].type == frontend::TokenType::SEMICN){ // 进入分支';'
        new Term(token_stream[index], result);                           // 匹配';'，加入语法树
        ++index;
        return true;
    }
    if(!lval && !block && !keyword && !exp && !semicn){
        return false;                                                    // 全部匹配失败，回溯
    }
    return true;
}

// Exp -> AddExp
bool Parser::parseExp(AstNode *parent){
    Exp *result = new Exp(parent);
    log(result);
    if(!parseAddExp(result)){
        return false;
    }
    return true;
}

// Cond -> LOrExp
bool Parser::parseCond(AstNode *parent){
    Cond *result = new Cond(parent);
    if(!parseLOrExp(result)){
        return false;
    }
    return true;
}

// LVal -> Ident {'[' Exp ']'}
bool Parser::parseLVal(AstNode *parent){
    LVal *result = new LVal(parent);
    log(result);
    if(token_stream[index].type != frontend::TokenType::IDENFR){
        return false;                                              // 匹配ident错误，回溯
    }
    new Term(token_stream[index], result);
    ++index;
    // 出现"{...}"，0~n次匹配，使用循环
    while(token_stream[index].type == frontend::TokenType::LBRACK){
        new Term(token_stream[index], result);                     // 终结符匹配成功，加入语法树
        ++index;
        if(!parseExp(result)){                                     // 进入exp分支
            return false;
        }
        if(token_stream[index].type != frontend::TokenType::RBRACK){
            return false;
        }
        new Term(token_stream[index], result);
        ++index;
    }
    return true;
}

// Number -> IntConst | floatConst
bool Parser::parseNumber(AstNode *parent){
    Number *result = new Number(parent);
    log(result);
    // 虽然存在分支，但是是终结符匹配，直接进行
    if(token_stream[index].type == frontend::TokenType::INTLTR){
        new Term(token_stream[index], result);
        ++index;
    }
    else if(token_stream[index].type == frontend::TokenType::FLOATLTR){
        new Term(token_stream[index], result);
        ++index;
    }
    else{
        return false;
    }
    return true;
}

// PrimaryExp -> '(' Exp ')' | LVal | Number
bool Parser::parsePrimaryExp(AstNode *parent){
    PrimaryExp *result = new PrimaryExp(parent);
    log(result);
    // 存在多个分支，标注分支
    bool lparent = false;                                           // iparent分支
    bool lval = false;                                              // lval分支
    bool number = false;                                            // number分支
    int last_index = index;
    int last_child_nums = result->children.size();
    if(token_stream[index].type == frontend::TokenType::LPARENT){   // 进入'(' Exp ')'
        new Term(token_stream[index], result);                      // 匹配'('
        ++index;
        lparent = parseExp(result);                                 // 匹配Exp
        if(lparent && token_stream[index].type == frontend::TokenType::RPARENT){
            new Term(token_stream[index], result);                  // 匹配')'
            ++index;
            return true;
        }else{
            lparent = false;                                        // 当前分支匹配出错
            retreat(last_index, result, last_child_nums);
        }
    }
    if(!lparent && Match_LVal().count(token_stream[index].type)){   // 进入分支LVal
        lval = parseLVal(result);
        if(!lval){
            retreat(last_index, result, last_child_nums);           // 清除当前匹配的子节点
        }
        else{
            return true;
        }
    }
    if(!lparent && !lval && Match_Number().count(token_stream[index].type)){ // 进入分支Number
        number = parseNumber(result);
        if(!number){
            retreat(last_index, result, last_child_nums);           // 清除当前匹配的子节点
        }
        else{
            return true;
        }
    }
    if(!lparent && !lval && !number){                               // 全部分支匹配出错，回溯
        return false;
    }
    return true;
}

// UnaryExp -> PrimaryExp | Ident '(' [FuncRParams] ')' | UnaryOp UnaryExp
bool Parser::parseUnaryExp(AstNode *parent){                       // 此函数采用LL(2)，因此本函数中无视分支
    UnaryExp *result = new UnaryExp(parent);
    log(result);
    if(token_stream[index].type == frontend::TokenType::IDENFR){   // 进入分支Ident '(' or PrimaryExp
        if(token_stream[index + 1].type == frontend::TokenType::LPARENT){
            new Term(token_stream[index], result);                 // 匹配到Ident '('
            ++index;
            new Term(token_stream[index], result);
            ++index;
            // 出现"[...]", 分支可选FuncRParams
            if(Match_FuncRParams().count(token_stream[index].type)){
                if(!parseFuncRParams(result)){
                    return false;
                }
            }
            if(token_stream[index].type != frontend::TokenType::RPARENT){
                return false;
            }
            new Term(token_stream[index], result);                  // 匹配')'
            ++index;
            return true;
        }
        else{                                                       // 分支匹配PrimaryExp
            if(!parsePrimaryExp(result)){
                return false;
            }
            return true;
        }
    }
    if(Match_PrimaryExp().count(token_stream[index].type)){         // 进入分支PrimaryExp
        if(!parsePrimaryExp(result)){
            return false;
        }
        return true;
    }
    if(Match_UnaryOp().count(token_stream[index].type)){            // 进入分支UnaryOp UnaryExp
        if(!parseUnaryOp(result)){
            return false;
        }
        if(!parseUnaryExp(result)){
            return false;
        }
        return true;
    }
    return false;
}

// UnaryOp -> '+' | '-' | '!'
bool Parser::parseUnaryOp(AstNode *parent){
    UnaryOp *result = new UnaryOp(parent);
    log(result);
    // 有"|"多选一, 且目前只有终结符，可以整合条件，直接判断
    if(token_stream[index].type != frontend::TokenType::PLUS && token_stream[index].type != frontend::TokenType::MINU && token_stream[index].type != frontend::TokenType::NOT){
        return false;
    }
    new Term(token_stream[index], result);
    ++index;
    return true;
}

// FuncRParams -> Exp { ',' Exp }
bool Parser::parseFuncRParams(AstNode *parent){
    FuncRParams *result = new FuncRParams(parent);
    log(result);
    if(!parseExp(result)){
        return false;
    }
    // 进入分支"{...}"，出现0~n次，使用循环
    while(token_stream[index].type == frontend::TokenType::COMMA){
        new Term(token_stream[index], result);
        ++index;
        if(!parseExp(result)){
            return false;
        }
    }
    return true;
}

// MulExp -> UnaryExp { ('*' | '/' | '%') UnaryExp }
bool Parser::parseMulExp(AstNode *parent){
    MulExp *result = new MulExp(parent);
    log(result);
    if(!parseUnaryExp(result)){
        return false;
    }
    // 进入分支"{...}"，出现0~n次，使用循环
    // 而且(...)内为一个整体，且有"|"多选一
    while(token_stream[index].type == frontend::TokenType::MULT || token_stream[index].type == frontend::TokenType::DIV || token_stream[index].type == frontend::TokenType::MOD){
        new Term(token_stream[index], result);
        ++index;
        if(!parseUnaryExp(result)){
            return false;
        }
    }
    return true;
}

// AddExp -> MulExp { ('+' | '-') MulExp }
bool Parser::parseAddExp(AstNode *parent){
    AddExp *result = new AddExp(parent);
    log(result);
    if(!parseMulExp(result)){
        return false;
    }
    // 进入分支"{...}"，出现0~n次，使用循环
    // 而且(...)内为一个整体，且有"|"多选一
    while(token_stream[index].type == frontend::TokenType::PLUS || token_stream[index].type == frontend::TokenType::MINU){
        new Term(token_stream[index], result);
        ++index;
        if(!parseMulExp(result)){
            return false;
        }
    }
    return true;
}

// RelExp -> AddExp { ('<' | '>' | '<=' | '>=') AddExp }
bool Parser::parseRelExp(AstNode *parent){
    RelExp *result = new RelExp(parent);
    log(result);
    if(!parseAddExp(result)){
        return false;
    }
    // 进入分支"{...}"，出现0~n次，使用循环
    // 而且(...)内为一个整体，且有"|"多选一
    while(token_stream[index].type == frontend::TokenType::LSS || token_stream[index].type == frontend::TokenType::GTR || token_stream[index].type == frontend::TokenType::LEQ || token_stream[index].type == frontend::TokenType::GEQ){
        new Term(token_stream[index], result);
        ++index;
        if(!parseAddExp(result)){
            return false;
        }
    }
    return true;
}

// EqExp -> RelExp { ('==' | '!=') RelExp }
bool Parser::parseEqExp(AstNode *parent){
    EqExp *result = new EqExp(parent);
    log(result);
    if(!parseRelExp(result)){
        return false;
    }
    // 进入分支"{...}"，出现0~n次，使用循环
    // 而且(...)内为一个整体，且有"|"多选一
    while(token_stream[index].type == frontend::TokenType::EQL || token_stream[index].type == frontend::TokenType::NEQ){
        new Term(token_stream[index], result);
        ++index;
        if(!parseRelExp(result)){
            return false;
        }
    }
    return true;
}

// LAndExp -> EqExp [ '&&' LAndExp ]
bool Parser::parseLAndExp(AstNode *parent){
    LAndExp *result = new LAndExp(parent);
    log(result);
    if(!parseEqExp(result)){
        return false;
    }
    // 进入可选分支"[...]"，出现0~1次，直接匹配
    if(token_stream[index].type == frontend::TokenType::AND){
        new Term(token_stream[index], result);
        ++index;
        if(!parseLAndExp(result)){
            return false;
        }
    }
    return true;
}

// LOrExp -> LAndExp [ '||' LOrExp ]
bool Parser::parseLOrExp(AstNode *parent){
    LOrExp *result = new LOrExp(parent);
    log(result);
    if(!parseLAndExp(result)){
        return false;
    }
    // 进入可选分支"[...]"，出现0~1次，直接匹配
    if(token_stream[index].type == frontend::TokenType::OR){
        new Term(token_stream[index], result);
        ++index;
        if(!parseLOrExp(result)){
            return false;
        }
    }
    return true;
}

// ConstExp -> AddExp
bool Parser::parseConstExp(AstNode *parent){
    ConstExp *result = new ConstExp(parent);
    log(result);
    if(!parseAddExp(result)){
        return false;
    }
    return true;
}

//辅助函数如下，收集first集，进行匹配
//CompUnit -> (Decl | FuncDef) [CompUnit]
std::unordered_set<frontend::TokenType> Parser::Match_Compunit(){
    auto first_decl = Match_Decl();              // 递归调用，获得first集
    auto first_funcdef = Match_FuncDef();        // 同理递归
    first_funcdef.insert(first_decl.begin(), first_decl.end());  // 合并first集
    return first_funcdef;                        // 返回两个集合的并集
}
//以下first集匹配同理

//Decl -> ConstDecl | VarDecl
std::unordered_set<frontend::TokenType> Parser::Match_Decl(){
    auto first_constdecl = Match_ConstDecl();
    auto first_vardecl = Match_VarDecl();
    first_vardecl.insert(first_constdecl.begin(), first_constdecl.end());
    return first_vardecl;       // 返回两个集合的并集
}

//ConstDecl -> 'const' BType ConstDef { ',' ConstDef } ';'
std::unordered_set<frontend::TokenType> Parser::Match_ConstDecl(){
    return {frontend::TokenType::CONSTTK};  // 返回'const'
}

//BType -> 'int' | 'float'
std::unordered_set<frontend::TokenType> Parser::Match_BType(){
    return {frontend::TokenType::INTTK, frontend::TokenType::FLOATTK};  // 返回'int'、'float'
}

//ConstDef -> Ident { '[' ConstExp ']' } '=' ConstInitVal
std::unordered_set<frontend::TokenType> Parser::Match_ConstDef(){
    return {frontend::TokenType::IDENFR};   // 返回Ident
}

//ConstInitVal -> ConstExp | '{' [ ConstInitVal { ',' ConstInitVal } ] '}'
std::unordered_set<frontend::TokenType> Parser::Match_ConstInitVal(){
    auto first_constexp = Match_ConstExp();
    first_constexp.insert(frontend::TokenType::LBRACE);
    return first_constexp;      // 返回ConstExp、'{'
}

//VarDecl -> BType VarDef { ',' VarDef } ';'
std::unordered_set<frontend::TokenType> Parser::Match_VarDecl(){
    return Match_BType();       // 返回BType
}

//VarDef -> Ident { '[' ConstExp ']' } [ '=' InitVal ]
std::unordered_set<frontend::TokenType> Parser::Match_VarDef(){
    return {frontend::TokenType::IDENFR};   // 返回Ident
}

//InitVal -> Exp | '{' [ InitVal { ',' InitVal } ] '}'
std::unordered_set<frontend::TokenType> Parser::Match_InitVal(){
    auto first_exp = Match_Exp();
    first_exp.insert(frontend::TokenType::LBRACE);
    return first_exp;           // 返回Exp、'{'
}

//FuncDef -> FuncType Ident '(' [FuncFParams] ')' Block
std::unordered_set<frontend::TokenType> Parser::Match_FuncDef(){
    return Match_FuncType();    // 返回FuncType
}

//FuncType -> 'void' | 'int' | 'float'
std::unordered_set<frontend::TokenType> Parser::Match_FuncType(){
    return {frontend::TokenType::VOIDTK, frontend::TokenType::INTTK, frontend::TokenType::FLOATTK}; // 返回'void'、'int'、'float'
}

//FuncFParam -> BType Ident ['[' ']' { '[' Exp ']' }]
std::unordered_set<frontend::TokenType> Parser::Match_FuncFParam(){
    return Match_BType();       // 返回BType
}

//FuncFParams -> FuncFParam { ',' FuncFParam }
std::unordered_set<frontend::TokenType> Parser::Match_FuncFParams(){
    return Match_FuncFParam();  // 返回FuncFParam
}

//Block -> '{' { BlockItem } '}'
std::unordered_set<frontend::TokenType> Parser::Match_Block(){
    return {frontend::TokenType::LBRACE};   // 返回'{'
}

//BlockItem -> Decl | Stmt
std::unordered_set<frontend::TokenType> Parser::Match_BlockItem(){
    auto first_decl = Match_Decl();
    auto first_stmt = Match_Stmt();
    first_stmt.insert(first_decl.begin(), first_decl.end());
    return first_stmt;          // 返回Decl、Stmt
}

//Stmt -> LVal '=' Exp ';' | Block | 'if' '(' Cond ')' Stmt [ 'else' Stmt ] | 'while' '(' Cond ')' Stmt | 'break' ';' | 'continue' ';' | 'return' [Exp] ';' | [Exp] ';'
std::unordered_set<frontend::TokenType> Parser::Match_Stmt(){
    auto first_lval = Match_LVal();
    auto first_block = Match_Block();
    first_lval.insert(first_block.begin(), first_block.end());
    first_lval.insert(frontend::TokenType::IFTK);
    first_lval.insert(frontend::TokenType::WHILETK);
    first_lval.insert(frontend::TokenType::BREAKTK);
    first_lval.insert(frontend::TokenType::CONTINUETK);
    first_lval.insert(frontend::TokenType::RETURNTK);
    auto first_exp = Match_Exp();
    first_lval.insert(first_exp.begin(), first_exp.end());
    first_lval.insert(frontend::TokenType::SEMICN);     // 因为[Exp]是可选项，所以';'也可能作为开头
    return first_lval;          // 返回LVal、Block、'if'、'while'、'break'、'continue'、'return'、[Exp]、';'
}

//Exp -> AddExp
std::unordered_set<frontend::TokenType> Parser::Match_Exp(){
    return Match_AddExp();      // 返回AddExp
}

//Cond -> LOrExp
std::unordered_set<frontend::TokenType> Parser::Match_Cond(){
    return Match_LOrExp();      // 返回LOrExp
}

//LVal -> Ident {'[' Exp ']'}
std::unordered_set<frontend::TokenType> Parser::Match_LVal(){
    return {frontend::TokenType::IDENFR};   // 返回Ident
}

//Number -> IntConst | floatConst
std::unordered_set<frontend::TokenType> Parser::Match_Number(){
    return {frontend::TokenType::INTLTR, frontend::TokenType::FLOATLTR};    // 返回IntConst、floatConst
}

//PrimaryExp -> '(' Exp ')' | LVal | Number
std::unordered_set<frontend::TokenType> Parser::Match_PrimaryExp(){
    auto first_lval = Match_LVal();
    auto first_number = Match_Number();
    first_lval.insert(first_number.begin(), first_number.end());
    first_lval.insert(frontend::TokenType::LPARENT);
    return first_lval;          // 返回'('、LVal、Number
}

//UnaryExp -> PrimaryExp | Ident '(' [FuncRParams] ')' | UnaryOp UnaryExp
std::unordered_set<frontend::TokenType> Parser::Match_UnaryExp(){
    auto first_primaryexp = Match_PrimaryExp();
    first_primaryexp.insert(frontend::TokenType::IDENFR);
    auto first_unaryop = Match_UnaryOp();
    first_primaryexp.insert(first_unaryop.begin(), first_unaryop.end());
    return first_primaryexp;    // 返回PrimaryExp、Ident、UnaryOp
}

//UnaryOp -> '+' | '-' | '!'
std::unordered_set<frontend::TokenType> Parser::Match_UnaryOp(){
    return {frontend::TokenType::PLUS, frontend::TokenType::MINU, frontend::TokenType::NOT};    // '+'、'-'、'!'
}

//FuncRParams -> Exp { ',' Exp }
std::unordered_set<frontend::TokenType> Parser::Match_FuncRParams(){
    return Match_Exp();         // 返回Exp
}

//MulExp -> UnaryExp { ('*' | '/' | '%') UnaryExp }
std::unordered_set<frontend::TokenType> Parser::Match_MulExp(){
    return Match_UnaryExp();    // 返回UnaryExp
}

//AddExp -> MulExp { ('+' | '-') MulExp }
std::unordered_set<frontend::TokenType> Parser::Match_AddExp(){
    return Match_MulExp();      // 返回MulExp
}

//RelExp -> AddExp { ('<' | '>' | '<=' | '>=') AddExp }
std::unordered_set<frontend::TokenType> Parser::Match_RelExp(){
    return Match_AddExp();      // 返回AddExp
}

//EqExp -> RelExp { ('==' | '!=') RelExp }
std::unordered_set<frontend::TokenType> Parser::Match_EqExp(){
    return Match_RelExp();      // 返回RelExp
}

//LAndExp -> EqExp [ '&&' LAndExp ]
std::unordered_set<frontend::TokenType> Parser::Match_LAndExp(){
    return Match_EqExp();       // 返回EqExp
}

//LOrExp -> LAndExp [ '||' LOrExp ]
std::unordered_set<frontend::TokenType> Parser::Match_LOrExp(){
    return Match_LAndExp();     // 返回LAndExp
}

//ConstExp -> AddExp
std::unordered_set<frontend::TokenType> Parser::Match_ConstExp(){
    return Match_AddExp();      // 返回AddExp
}


// 辅助函数，撤回上一步匹配
void Parser::retreat(int last_index, AstNode *curr_result, int last_child_nums){
    index = last_index;         // 恢复匹配前的index
    while (int(curr_result->children.size()) > last_child_nums){
        curr_result->children.pop_back();                   // 恢复到进入分支前的children数量
    }
    return;
}