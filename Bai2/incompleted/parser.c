/* 
 * @copyright (c) 2008, Hedspi, Hanoi University of Technology
 * @author Huu-Duc Nguyen
 * @version 1.0
 */

#include <stdlib.h>

#include "reader.h"
#include "scanner.h"
#include "parser.h"
#include "error.h"

Token *currentToken;
Token *lookAhead;

void scan(void) {
  Token* tmp = currentToken;
  currentToken = lookAhead;
  lookAhead = getValidToken();
  free(tmp);
}

void eat(TokenType tokenType) {
  if (lookAhead->tokenType == tokenType) {
    printToken(lookAhead);
    scan();
  } else missingToken(tokenType, lookAhead->lineNo, lookAhead->colNo);
}

void compileProgram(void) {
  assert("Parsing a Program ....");
  eat(KW_PROGRAM);
  eat(TK_IDENT);
  eat(SB_SEMICOLON);
  compileBlock();
  eat(SB_PERIOD);
  assert("Program parsed!");
}

void compileBlock(void) {
  assert("Parsing a Block ....");
  if (lookAhead->tokenType == KW_CONST) {
    eat(KW_CONST);
    compileConstDecl();
    compileConstDecls();
    compileBlock2();
  } 
  else compileBlock2();
  assert("Block parsed!");
}

void compileBlock2(void) {
  if (lookAhead->tokenType == KW_TYPE) {
    eat(KW_TYPE);
    compileTypeDecl();
    compileTypeDecls();
    compileBlock3();
  } 
  else compileBlock3();
}

void compileBlock3(void) {
  if (lookAhead->tokenType == KW_VAR) {
    eat(KW_VAR);
    compileVarDecl();
    compileVarDecls();
    compileBlock4();
  } 
  else compileBlock4();
}

void compileBlock4(void) {
  compileSubDecls();
  compileBlock5();
}

void compileBlock5(void) {
  eat(KW_BEGIN);
  compileStatements();
  eat(KW_END);
}

void compileConstDecls(void) {
  // BNF: ConstDecls ::= ConstDecl ConstDecls | epsilon
  if (lookAhead->tokenType == TK_IDENT) {
    compileConstDecl();
    compileConstDecls();
  }
  // else: epsilon (không làm gì)
}

void compileConstDecl(void) {
  // BNF: ConstDecl ::= Ident = Constant ;
  eat(TK_IDENT);
  eat(SB_EQ);
  compileConstant();
  eat(SB_SEMICOLON);
}

void compileTypeDecls(void) {
  // BNF: TypeDecls ::= TypeDecl TypeDecls | epsilon
  if (lookAhead->tokenType == TK_IDENT) {
    compileTypeDecl();
    compileTypeDecls();
  }
}

void compileTypeDecl(void) {
  // BNF: TypeDecl ::= Ident = Type ;
  eat(TK_IDENT);
  eat(SB_EQ);
  compileType();
  eat(SB_SEMICOLON);
}

void compileVarDecls(void) {
  // BNF: VarDecls ::= VarDecl VarDecls | epsilon
  if (lookAhead->tokenType == TK_IDENT) {
    compileVarDecl();
    compileVarDecls();
  }
}

void compileVarDecl(void) {
  // BNF: VarDecl ::= Ident : Type ;
  eat(TK_IDENT);
  eat(SB_COLON);
  compileType();
  eat(SB_SEMICOLON);
}

void compileSubDecls(void) {
  assert("Parsing subtoutines ....");
  
  // Lặp liên tục chừng nào còn nhìn thấy FUNCTION hoặc PROCEDURE
  while (lookAhead->tokenType == KW_FUNCTION || lookAhead->tokenType == KW_PROCEDURE) {
    if (lookAhead->tokenType == KW_FUNCTION) {
      compileFuncDecl();
    } else {
      compileProcDecl();
    }
  }
  
  assert("Subtoutines parsed ....");
}

void compileFuncDecl(void) {
  assert("Parsing a function ....");
  eat(KW_FUNCTION);
  eat(TK_IDENT);
  compileParams();
  eat(SB_COLON);
  compileBasicType();
  eat(SB_SEMICOLON);
  compileBlock();
  eat(SB_SEMICOLON);
  assert("Function parsed ....");
}

void compileProcDecl(void) {
  assert("Parsing a procedure ....");
  eat(KW_PROCEDURE);
  eat(TK_IDENT);
  compileParams();
  eat(SB_SEMICOLON);
  compileBlock();
  eat(SB_SEMICOLON);
  assert("Procedure parsed ....");
}

void compileUnsignedConstant(void) {
  // BNF: UnsignedConstant ::= Number | ConstIdent | ConstChar | String
  switch (lookAhead->tokenType) {
  case TK_NUMBER:
    eat(TK_NUMBER);
    break;
  case TK_IDENT:
    eat(TK_IDENT);
    break;
  case TK_CHAR:
    eat(TK_CHAR);
    break;
  case TK_STRING: // MỚI: Hỗ trợ hằng chuỗi
    eat(TK_STRING);
    break;
  default:
    error(ERR_INVALIDCONSTANT, lookAhead->lineNo, lookAhead->colNo);
    break;
  }
}

void compileConstant(void) {
  // BNF: Constant ::= + Constant2 | - Constant2 | Constant2
  switch (lookAhead->tokenType) {
  case SB_PLUS:
    eat(SB_PLUS);
    compileConstant2();
    break;
  case SB_MINUS:
    eat(SB_MINUS);
    compileConstant2();
    break;
  case TK_CHAR:
  case TK_NUMBER:
  case TK_IDENT:
  case TK_STRING: // MỚI
    compileConstant2();
    break;
  default:
    error(ERR_INVALIDCONSTANT, lookAhead->lineNo, lookAhead->colNo);
    break;
  }
}

void compileConstant2(void) {
  // BNF: Constant2 ::= Ident | Number | Char | String
  switch (lookAhead->tokenType) {
  case TK_IDENT:
    eat(TK_IDENT);
    break;
  case TK_NUMBER:
    eat(TK_NUMBER);
    break;
  case TK_CHAR:
    eat(TK_CHAR);
    break;
  case TK_STRING: // MỚI
    eat(TK_STRING);
    break;
  default:
    error(ERR_INVALIDCONSTANT, lookAhead->lineNo, lookAhead->colNo);
    break;
  }
}

void compileType(void) {
  // BNF: Type ::= KW_INTEGER | KW_CHAR | KW_STRING | KW_BYTES | TypeIdent | ArrayType
  switch (lookAhead->tokenType) {
  case KW_INTEGER:
    eat(KW_INTEGER);
    break;
  case KW_CHAR:
    eat(KW_CHAR);
    break;
  case KW_STRING: // MỚI
    eat(KW_STRING);
    break;
  case KW_BYTES: // MỚI
    eat(KW_BYTES);
    break;
  case TK_IDENT:
    eat(TK_IDENT);
    break;
  case KW_ARRAY:
    eat(KW_ARRAY);
    eat(SB_LSEL);
    eat(TK_NUMBER);
    eat(SB_RSEL);
    eat(KW_OF);
    compileType();
    break;
  default:
    error(ERR_INVALIDTYPE, lookAhead->lineNo, lookAhead->colNo);
    break;
  }
}

void compileBasicType(void) {
  // BNF: BasicType ::= INTEGER | CHAR | STRING | BYTES
  switch (lookAhead->tokenType) {
  case KW_INTEGER:
    eat(KW_INTEGER);
    break;
  case KW_CHAR:
    eat(KW_CHAR);
    break;
  case KW_STRING: // MỚI
    eat(KW_STRING);
    break;
  case KW_BYTES: // MỚI
    eat(KW_BYTES);
    break;
  default:
    error(ERR_INVALIDBASICTYPE, lookAhead->lineNo, lookAhead->colNo);
    break;
  }
}

void compileParams(void) {
  // BNF: Params ::= ( Param Params2 ) | epsilon
  if (lookAhead->tokenType == SB_LPAR) {
    eat(SB_LPAR);
    compileParam();
    compileParams2();
    eat(SB_RPAR);
  }
}

void compileParams2(void) {
  // BNF: Params2 ::= ; Param Params2 | epsilon
  if (lookAhead->tokenType == SB_SEMICOLON) {
    eat(SB_SEMICOLON);
    compileParam();
    compileParams2();
  }
}

void compileParam(void) {
  // BNF: Param ::= Ident : BasicType | VAR Ident : BasicType
  if (lookAhead->tokenType == TK_IDENT) {
    eat(TK_IDENT);
    eat(SB_COLON);
    compileBasicType();
  } else if (lookAhead->tokenType == KW_VAR) {
    eat(KW_VAR);
    eat(TK_IDENT);
    eat(SB_COLON);
    compileBasicType();
  } else {
    error(ERR_INVALIDPARAM, lookAhead->lineNo, lookAhead->colNo);
  }
}

void compileStatements(void) {
  // BNF: Statements ::= Statement Statements2
  compileStatement();
  compileStatements2();
}

void compileStatements2(void) {
  // BNF: Statements2 ::= ; Statement Statements2 | epsilon
  if (lookAhead->tokenType == SB_SEMICOLON) {
    eat(SB_SEMICOLON);
    compileStatement();
    compileStatements2();
  }
  else {
    // XỬ LÝ LỖI THIẾU CHẤM PHẨY:
    // Nếu không thấy dấu chấm phẩy, nhưng lại thấy bắt đầu của một câu lệnh mới
    // --> Nghĩa là thiếu dấu chấm phẩy ngăn cách.
    if (lookAhead->tokenType == KW_CALL || 
        lookAhead->tokenType == TK_IDENT || 
        lookAhead->tokenType == KW_IF || 
        lookAhead->tokenType == KW_WHILE || 
        lookAhead->tokenType == KW_FOR || 
        lookAhead->tokenType == KW_REPEAT || // Cấu trúc mới thêm
        lookAhead->tokenType == KW_BEGIN) {
        
        eat(SB_SEMICOLON); // Lệnh này sẽ kích hoạt error: "Missing ';'" và dừng chương trình
    }
    
    // Nếu không phải các trường hợp trên, ta mới coi là epsilon (Hết danh sách)
    // Trường hợp đúng: Gặp KW_END hoặc KW_ELSE
  }
}

// MỚI: Hàm xử lý lệnh REPEAT ... UNTIL
void compileRepeatSt(void) {
  assert("Parsing a repeat statement ....");
  eat(KW_REPEAT);
  compileStatements();
  eat(KW_UNTIL);
  compileCondition();
  assert("Repeat statement parsed ....");
}

void compileStatement(void) {
  switch (lookAhead->tokenType) {
  case TK_IDENT:
    compileAssignSt();
    break;
  case KW_CALL:
    compileCallSt();
    break;
  case KW_BEGIN:
    compileGroupSt();
    break;
  case KW_IF:
    compileIfSt();
    break;
  case KW_WHILE:
    compileWhileSt();
    break;
  case KW_FOR:
    compileForSt();
    break;
  case KW_REPEAT: // MỚI
    compileRepeatSt();
    break;
    // EmptySt needs to check FOLLOW tokens
  case SB_SEMICOLON:
  case KW_END:
  case KW_ELSE:
  case KW_UNTIL: // MỚI
    break;
    // Error occurs
  default:
    error(ERR_INVALIDSTATEMENT, lookAhead->lineNo, lookAhead->colNo);
    break;
  }
}

void compileAssignSt(void) {
  assert("Parsing an assign statement ....");
  
  // --- PHẦN 1: VẾ TRÁI (LEFT-HAND SIDE) ---
  
  // 1.1. Đọc biến đầu tiên
  // Variable ::= Ident [Indexes]
  eat(TK_IDENT);
  if (lookAhead->tokenType == SB_LSEL) {
    compileIndexes();
  }

  // 1.2. Vòng lặp: Nếu thấy dấu phẩy thì tiếp tục đọc biến tiếp theo
  while (lookAhead->tokenType == SB_COMMA) {
    eat(SB_COMMA); // Ăn dấu ,
    
    eat(TK_IDENT); // Ăn tên biến tiếp theo
    if (lookAhead->tokenType == SB_LSEL) {
      compileIndexes(); // Ăn chỉ số mảng (nếu có)
    }
  }

  // --- PHẦN 2: DẤU GÁN ---
  eat(SB_ASSIGN);

  // --- PHẦN 3: VẾ PHẢI (RIGHT-HAND SIDE) ---

  // 3.1. Đọc biểu thức đầu tiên
  compileExpression();

  // 3.2. Vòng lặp: Nếu thấy dấu phẩy thì tiếp tục đọc biểu thức tiếp theo
  while (lookAhead->tokenType == SB_COMMA) {
    eat(SB_COMMA); // Ăn dấu ,
    compileExpression(); // Phân tích biểu thức tiếp theo
  }

  assert("Assign statement parsed ....");
}

void compileCallSt(void) {
  assert("Parsing a call statement ....");
  eat(KW_CALL);
  eat(TK_IDENT);
  compileArguments();
  assert("Call statement parsed ....");
}

void compileGroupSt(void) {
  assert("Parsing a group statement ....");
  eat(KW_BEGIN);
  compileStatements();
  eat(KW_END);
  assert("Group statement parsed ....");
}

void compileIfSt(void) {
  assert("Parsing an if statement ....");
  eat(KW_IF);
  compileCondition();
  eat(KW_THEN);
  compileStatement();
  if (lookAhead->tokenType == KW_ELSE) 
    compileElseSt();
  assert("If statement parsed ....");
}

void compileElseSt(void) {
  eat(KW_ELSE);
  compileStatement();
}

void compileWhileSt(void) {
  assert("Parsing a while statement ....");
  eat(KW_WHILE);
  compileCondition();
  eat(KW_DO);
  compileStatement();
  assert("While statement parsed ....");
}

void compileForSt(void) {
  assert("Parsing a for statement ....");
  eat(KW_FOR);
  eat(TK_IDENT);
  eat(SB_ASSIGN);
  compileExpression();
  eat(KW_TO);
  compileExpression();
  eat(KW_DO);
  compileStatement();
  assert("For statement parsed ....");
}

void compileCondition(void) {
  // BNF: Condition ::= Expression Condition2
  compileExpression();
  compileCondition2();
}

void compileCondition2(void) {
  // BNF: Condition2 ::= = Expr | != Expr | ...
  switch (lookAhead->tokenType) {
  case SB_EQ:
    eat(SB_EQ);
    compileExpression();
    break;
  case SB_NEQ:
    eat(SB_NEQ);
    compileExpression();
    break;
  case SB_LE:
    eat(SB_LE);
    compileExpression();
    break;
  case SB_LT:
    eat(SB_LT);
    compileExpression();
    break;
  case SB_GE:
    eat(SB_GE);
    compileExpression();
    break;
  case SB_GT:
    eat(SB_GT);
    compileExpression();
    break;
  default:
    error(ERR_INVALIDCOMPARATOR, lookAhead->lineNo, lookAhead->colNo);
    break;
  }
}

void compileArguments(void) {
  // BNF: Arguments ::= ( Expression Arguments2 ) | epsilon
  if (lookAhead->tokenType == SB_LPAR) {
    eat(SB_LPAR);
    compileExpression();
    compileArguments2();
    eat(SB_RPAR);
  }
}

void compileArguments2(void) {
  // BNF: Arguments2 ::= , Expression Arguments2 | epsilon
  if (lookAhead->tokenType == SB_COMMA) {
    eat(SB_COMMA);
    compileExpression();
    compileArguments2();
  }
}

void compileExpression(void) {
  assert("Parsing an expression");
  // BNF: Expression ::= + Expression2 | - Expression2 | Expression2
  switch (lookAhead->tokenType) {
  case SB_PLUS:
    eat(SB_PLUS);
    compileExpression2();
    break;
  case SB_MINUS:
    eat(SB_MINUS);
    compileExpression2();
    break;
  default:
    compileExpression2();
    break;
  }
  assert("Expression parsed");
}

void compileExpression2(void) {
  // BNF: Expression2 ::= Term Expression3
  compileTerm();
  compileExpression3();
}

void compileExpression3(void) {
  // BNF: Expression3 ::= + Term Expression3 | - Term Expression3 | epsilon
  switch (lookAhead->tokenType) {
  case SB_PLUS:
    eat(SB_PLUS);
    compileTerm();
    compileExpression3();
    break;
  case SB_MINUS:
    eat(SB_MINUS);
    compileTerm();
    compileExpression3();
    break;
  // Follow set
  case SB_SEMICOLON:
  case KW_END:
  case KW_ELSE:
  case KW_THEN:
  case KW_DO:
  case KW_TO:
  case SB_RPAR:
  case SB_COMMA:
  case SB_RSEL:
  case SB_EQ:
  case SB_NEQ:
  case SB_LE:
  case SB_LT:
  case SB_GE:
  case SB_GT:
  case KW_UNTIL: // MỚI: Cho Repeat
    break;
  default:
    error(ERR_INVALIDEXPRESSION, lookAhead->lineNo, lookAhead->colNo);
    break;
  }
}

void compileTerm(void) {
  // BNF: Term ::= Factor Term2
  compileFactor();
  compileTerm2();
}

void compileTerm2(void) {
  // BNF: Term2 ::= * Factor Term2 | / Factor Term2 | % Factor Term2 | epsilon
  switch (lookAhead->tokenType) {
  case SB_TIMES:
    eat(SB_TIMES);
    compileFactor();
    compileTerm2();
    break;
  case SB_SLASH:
    eat(SB_SLASH);
    compileFactor();
    compileTerm2();
    break;
  case SB_MOD: // MỚI: Phép lấy dư
    eat(SB_MOD);
    compileFactor();
    compileTerm2();
    break;
  // Follow set (giống Expression3 + PLUS + MINUS)
  case SB_PLUS:
  case SB_MINUS:
  case SB_SEMICOLON:
  case KW_END:
  case KW_ELSE:
  case KW_THEN:
  case KW_DO:
  case KW_TO:
  case SB_RPAR:
  case SB_COMMA:
  case SB_RSEL:
  case SB_EQ:
  case SB_NEQ:
  case SB_LE:
  case SB_LT:
  case SB_GE:
  case SB_GT:
  case KW_UNTIL: // MỚI
    break;
  default:
    error(ERR_INVALIDTERM, lookAhead->lineNo, lookAhead->colNo);
    break;
  }
}

void compileFactor(void) {
  // BNF: Factor ::= Number | Char | String | Ident... | (Expr)
  switch (lookAhead->tokenType) {
  case TK_NUMBER:
  case TK_CHAR:
  case TK_STRING: // MỚI
    compileUnsignedConstant();
    break;
  case SB_LPAR:
    eat(SB_LPAR);
    compileExpression();
    eat(SB_RPAR);
    break;
  case TK_IDENT:
    eat(TK_IDENT);
    // Xử lý sự nhập nhằng LL(2) giữa Biến và Hàm
    switch (lookAhead->tokenType) {
    case SB_LSEL: // Variable (Array index)
      compileIndexes();
      break;
    case SB_LPAR: // Function Call
      compileArguments();
      break;
    default: // Variable (Simple)
      break;
    }
    break;
  default:
    error(ERR_INVALIDFACTOR, lookAhead->lineNo, lookAhead->colNo);
    break;
  }
  
  // MỚI: Xử lý phép lũy thừa (**)
  // Factor -> Base ** Factor | Base
  if (lookAhead->tokenType == SB_POWER) {
      eat(SB_POWER);
      compileFactor(); // Đệ quy để xử lý tính kết hợp phải (Right Associative)
  }
}

void compileIndexes(void) {
  // BNF: Indexes ::= [ Expr ] Indexes | epsilon
  if (lookAhead->tokenType == SB_LSEL) {
    eat(SB_LSEL);
    compileExpression();
    eat(SB_RSEL);
    compileIndexes();
  }
}

int compile(char *fileName) {
  if (openInputStream(fileName) == IO_ERROR)
    return IO_ERROR;

  currentToken = NULL;
  lookAhead = getValidToken();

  compileProgram();

  free(currentToken);
  free(lookAhead);
  closeInputStream();
  return IO_SUCCESS;

}