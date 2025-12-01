/* Scanner
 * @copyright (c) 2008, Hedspi, Hanoi University of Technology
 * @author Huu-Duc Nguyen
 * @version 1.0
 */

#include <stdio.h>
#include <stdlib.h>

#include "reader.h"
#include "charcode.h"
#include "token.h"
#include "error.h"
#include "scanner.h"


extern int lineNo;
extern int colNo;
extern int currentChar;

extern CharCode charCodes[];

/***************************************************************/

void skipBlank() {
  while (currentChar != EOF && charCodes[currentChar] == CHAR_SPACE) {
    readChar();
  }
}

void skipComment() {
  int state = 0;
  while (currentChar != EOF && state < 2) {
    switch (charCodes[currentChar]) {
    case CHAR_TIMES: // Gặp dấu *
      state = 1;
      break;
    case CHAR_RPAR: // Gặp dấu )
      if (state == 1) {
        readChar(); // <--- SỬA LỖI: Đọc qua dấu ) để kết thúc comment hoàn toàn
        return;
      }
      state = 0;
      break;
    default:
      state = 0;
    }
    readChar();
  }
  // Nếu kết thúc vòng lặp mà chưa gặp *) thì là lỗi EOF
  if (currentChar == EOF) {
    error(ERR_ENDOFCOMMENT, lineNo, colNo);
  }
}

Token* readIdentKeyword(void) {
  Token *token;
  int ln = lineNo, cn = colNo;
  int count = 0;

  token = makeToken(TK_IDENT, ln, cn);

  while (currentChar != EOF && 
         (charCodes[currentChar] == CHAR_LETTER || charCodes[currentChar] == CHAR_DIGIT)) {
    // Chỉ lưu ký tự nếu chưa vượt quá độ dài tối đa
    if (count <= MAX_IDENT_LEN) {
      token->string[count] = (char)currentChar;
    }
    count++;
    readChar();
  }
  token->string[count > MAX_IDENT_LEN ? MAX_IDENT_LEN : count] = '\0';

  if (count > MAX_IDENT_LEN) {
    error(ERR_IDENTTOOLONG, ln, cn);
    return token;
  }

  // Kiểm tra xem định danh vừa đọc có phải là từ khóa không
  TokenType type = checkKeyword(token->string);
  if (type != TK_NONE) {
    token->tokenType = type;
  }

  return token;
}

Token* readNumber(void) {
  Token *token;
  int ln = lineNo, cn = colNo;
  int count = 0;

  token = makeToken(TK_NUMBER, ln, cn);

  while (currentChar != EOF && charCodes[currentChar] == CHAR_DIGIT) {
    // KPL không yêu cầu giới hạn độ dài số trong bài này, nhưng ta vẫn nên lưu vào string
    if (count <= MAX_IDENT_LEN) { // Tạm dùng MAX_IDENT_LEN cho số
        token->string[count] = (char)currentChar;
    }
    count++;
    readChar();
  }
  token->string[count > MAX_IDENT_LEN ? MAX_IDENT_LEN : count] = '\0';
  
  token->value = atoi(token->string);
  return token;
}

Token* readConstChar(void) {
  Token *token;
  int ln = lineNo, cn = colNo;
  
  readChar(); // Bỏ qua dấu nháy mở '
  
  if (currentChar == EOF) {
    error(ERR_INVALIDCHARCONSTANT, ln, cn);
    return makeToken(TK_NONE, ln, cn);
  }
  
  // Ký tự bên trong
  int charValue = currentChar;
  readChar(); 

  if (charCodes[currentChar] == CHAR_SINGLEQUOTE) {
    token = makeToken(TK_CHAR, ln, cn);
    token->string[0] = (char)charValue;
    token->string[1] = '\0';
    token->value = charValue;
    readChar(); // Bỏ qua dấu nháy đóng '
    return token;
  } else {
    error(ERR_INVALIDCHARCONSTANT, ln, cn);
    return makeToken(TK_NONE, ln, cn);
  }
}

Token* readString(void) {
  Token *token;
  int ln = lineNo, cn = colNo;
  int count = 0;

  token = makeToken(TK_STRING, ln, cn);
  readChar(); // Bỏ qua dấu " mở đầu

  while (currentChar != EOF && charCodes[currentChar] != CHAR_DOUBLEQUOTE) {
      if (count <= MAX_IDENT_LEN) { // Tận dụng MAX_IDENT_LEN hoặc tự định nghĩa MAX_STRING_LEN
          token->string[count] = (char)currentChar;
          count++;
      }
      // Nếu muốn xử lý ký tự thoát (escape) như \n, \" thì viết thêm code ở đây
      readChar();
  }
  token->string[count] = '\0';

  if (currentChar == EOF) {
      error(ERR_INVALIDSYMBOL, ln, cn); // Hoặc tạo lỗi mới ERR_UNTERMINATED_STRING
      return token;
  }

  readChar(); // Bỏ qua dấu " đóng
  return token;
}

void skipLineComment() {
  // Đọc liên tục cho đến khi gặp xuống dòng hoặc kết thúc file
  while (currentChar != EOF && currentChar != '\n') {
    readChar();
  }
  // Lưu ý: Không cần readChar() thêm lần nữa để ăn ký tự '\n' ở đây, 
  // vì hàm getToken() lần sau sẽ gọi skipBlank() và skipBlank() sẽ xử lý nó.
}

Token* getToken(void) {
  Token *token;
  int ln, cn;

  if (currentChar == EOF) 
    return makeToken(TK_EOF, lineNo, colNo);

  switch (charCodes[currentChar]) {
  case CHAR_SPACE: skipBlank(); return getToken();
  case CHAR_LETTER: return readIdentKeyword();
  case CHAR_DIGIT: return readNumber();
  
  case CHAR_PLUS: 
    token = makeToken(SB_PLUS, lineNo, colNo);
    readChar(); 
    return token;
    
  case CHAR_MINUS:
    token = makeToken(SB_MINUS, lineNo, colNo);
    readChar(); 
    return token;

  case CHAR_TIMES: // Xử lý * hoặc **
    ln = lineNo; cn = colNo;
    readChar(); // Đọc qua dấu * thứ nhất

    if (currentChar != EOF && charCodes[currentChar] == CHAR_TIMES) {
      // Nếu ký tự tiếp theo cũng là *, nghĩa là toán tử lũy thừa **
      token = makeToken(SB_POWER, ln, cn);
      readChar(); // Đọc qua dấu * thứ hai
    } else {
      // Nếu không, đây là phép nhân bình thường
      token = makeToken(SB_TIMES, ln, cn);
    }
    return token;

  case CHAR_SLASH:
    ln = lineNo; cn = colNo;
    readChar(); // Đã đọc dấu '/' thứ nhất

    if (currentChar != EOF && charCodes[currentChar] == CHAR_SLASH) {
      // Nếu ký tự tiếp theo cũng là '/', nghĩa là bắt đầu comment "//"
      readChar(); // Đọc bỏ dấu '/' thứ hai
      skipLineComment(); // Bỏ qua phần còn lại của dòng
      return getToken(); // Gọi đệ quy để lấy token tiếp theo
    } else {
      // Nếu không phải, thì đây là phép chia bình thường
      token = makeToken(SB_SLASH, ln, cn);
      // Lưu ý: Không gọi readChar() ở đây nữa vì ta đã gọi ở đầu case rồi,
      // biến currentChar hiện tại đang giữ ký tự tiếp theo sau dấu chia.
      return token;
    }

  case CHAR_LT: // Có thể là < hoặc <=
    ln = lineNo; cn = colNo;
    readChar();
    if (charCodes[currentChar] == CHAR_EQ) {
      token = makeToken(SB_LE, ln, cn);
      readChar();
    } else {
      token = makeToken(SB_LT, ln, cn);
    }
    return token;

  case CHAR_GT: // Có thể là > hoặc >=
    ln = lineNo; cn = colNo;
    readChar();
    if (charCodes[currentChar] == CHAR_EQ) {
      token = makeToken(SB_GE, ln, cn);
      readChar();
    } else {
      token = makeToken(SB_GT, ln, cn);
    }
    return token;

  case CHAR_EQ: 
    token = makeToken(SB_EQ, lineNo, colNo);
    readChar(); 
    return token;

  case CHAR_EXCLAIMATION: // Xử lý !=
    ln = lineNo; cn = colNo;
    readChar();
    if (charCodes[currentChar] == CHAR_EQ) {
      token = makeToken(SB_NEQ, ln, cn);
      readChar();
      return token;
    } else {
      error(ERR_INVALIDSYMBOL, ln, cn);
      return makeToken(TK_NONE, ln, cn);
    }

  case CHAR_COMMA:
    token = makeToken(SB_COMMA, lineNo, colNo);
    readChar(); 
    return token;

  case CHAR_PERIOD: // Có thể là . hoặc .) (RSEL)
    ln = lineNo; cn = colNo;
    readChar();
    if (charCodes[currentChar] == CHAR_RPAR) {
      token = makeToken(SB_RSEL, ln, cn);
      readChar();
    } else {
      token = makeToken(SB_PERIOD, ln, cn);
    }
    return token;

  case CHAR_SEMICOLON:
    token = makeToken(SB_SEMICOLON, lineNo, colNo);
    readChar(); 
    return token;

  case CHAR_COLON: // Có thể là : hoặc :=
    ln = lineNo; cn = colNo;
    readChar();
    if (charCodes[currentChar] == CHAR_EQ) {
      token = makeToken(SB_ASSIGN, ln, cn);
      readChar();
    } else {
      token = makeToken(SB_COLON, ln, cn);
    }
    return token;

  case CHAR_SINGLEQUOTE: 
    return readConstChar();

  // Xử lý chuỗi ký tự "..."
  case CHAR_DOUBLEQUOTE: 
      return readString();

  // Xử lý phép chia lấy dư %
  case CHAR_PERCENT:
      token = makeToken(SB_MOD, lineNo, colNo);
      readChar();
      return token;

  case CHAR_LPAR: // Có thể là (, (. (LSEL), hoặc (* (Comment)
    ln = lineNo; cn = colNo;
    readChar();
    
    if (currentChar == EOF) 
      return makeToken(SB_LPAR, ln, cn);

    switch (charCodes[currentChar]) {
    case CHAR_PERIOD: // (.
      token = makeToken(SB_LSEL, ln, cn);
      readChar();
      return token;
    case CHAR_TIMES: // (* -> Comment
      readChar(); // Bỏ qua *
      skipComment();
      return getToken(); // Gọi đệ quy để lấy token tiếp theo sau comment
    default:
      return makeToken(SB_LPAR, ln, cn);
    }

  case CHAR_RPAR:
    token = makeToken(SB_RPAR, lineNo, colNo);
    readChar(); 
    return token;

  default:
    token = makeToken(TK_NONE, lineNo, colNo);
    error(ERR_INVALIDSYMBOL, lineNo, colNo);
    readChar(); 
    return token;
  }
}

Token* getValidToken(void) {
  Token *token = getToken();
  while (token->tokenType == TK_NONE) {
    free(token);
    token = getToken();
  }
  return token;
}

/******************************************************************/

void printToken(Token *token) {

  printf("%d-%d:", token->lineNo, token->colNo);

  switch (token->tokenType) {
  case TK_NONE: printf("TK_NONE\n"); break;
  case TK_IDENT: printf("TK_IDENT(%s)\n", token->string); break;
  case TK_NUMBER: printf("TK_NUMBER(%s)\n", token->string); break;
  case TK_CHAR: printf("TK_CHAR(\'%s\')\n", token->string); break;
  case TK_EOF: printf("TK_EOF\n"); break;

  case KW_PROGRAM: printf("KW_PROGRAM\n"); break;
  case KW_CONST: printf("KW_CONST\n"); break;
  case KW_TYPE: printf("KW_TYPE\n"); break;
  case KW_VAR: printf("KW_VAR\n"); break;
  case KW_INTEGER: printf("KW_INTEGER\n"); break;
  case KW_CHAR: printf("KW_CHAR\n"); break;
  case KW_ARRAY: printf("KW_ARRAY\n"); break;
  case KW_OF: printf("KW_OF\n"); break;
  case KW_FUNCTION: printf("KW_FUNCTION\n"); break;
  case KW_PROCEDURE: printf("KW_PROCEDURE\n"); break;
  case KW_BEGIN: printf("KW_BEGIN\n"); break;
  case KW_END: printf("KW_END\n"); break;
  case KW_CALL: printf("KW_CALL\n"); break;
  case KW_IF: printf("KW_IF\n"); break;
  case KW_THEN: printf("KW_THEN\n"); break;
  case KW_ELSE: printf("KW_ELSE\n"); break;
  case KW_WHILE: printf("KW_WHILE\n"); break;
  case KW_DO: printf("KW_DO\n"); break;
  case KW_FOR: printf("KW_FOR\n"); break;
  case KW_TO: printf("KW_TO\n"); break;

  case SB_SEMICOLON: printf("SB_SEMICOLON\n"); break;
  case SB_COLON: printf("SB_COLON\n"); break;
  case SB_PERIOD: printf("SB_PERIOD\n"); break;
  case SB_COMMA: printf("SB_COMMA\n"); break;
  case SB_ASSIGN: printf("SB_ASSIGN\n"); break;
  case SB_EQ: printf("SB_EQ\n"); break;
  case SB_NEQ: printf("SB_NEQ\n"); break;
  case SB_LT: printf("SB_LT\n"); break;
  case SB_LE: printf("SB_LE\n"); break;
  case SB_GT: printf("SB_GT\n"); break;
  case SB_GE: printf("SB_GE\n"); break;
  case SB_PLUS: printf("SB_PLUS\n"); break;
  case SB_MINUS: printf("SB_MINUS\n"); break;
  case SB_TIMES: printf("SB_TIMES\n"); break;
  case SB_SLASH: printf("SB_SLASH\n"); break;
  case SB_LPAR: printf("SB_LPAR\n"); break;
  case SB_RPAR: printf("SB_RPAR\n"); break;
  case SB_LSEL: printf("SB_LSEL\n"); break;
  case SB_RSEL: printf("SB_RSEL\n"); break;
  case TK_STRING: printf("TK_STRING(\"%s\")\n", token->string); break; // <--- THÊM
  case KW_STRING: printf("KW_STRING\n"); break; // <--- THÊM
  case SB_MOD: printf("SB_MOD\n"); break;       // <--- THÊM
  case KW_BYTES: printf("KW_BYTES\n"); break; // <--- THÊM
  case SB_POWER: printf("SB_POWER\n"); break; // <--- THÊM
  case KW_REPEAT: printf("KW_REPEAT\n"); break; // <--- THÊM
  case KW_UNTIL: printf("KW_UNTIL\n"); break;   // <--- THÊM
  }
}

// int scan(char *fileName) {
//   Token *token;

//   if (openInputStream(fileName) == IO_ERROR)
//     return IO_ERROR;

//   token = getToken();
//   while (token->tokenType != TK_EOF) {
//     printToken(token);
//     free(token);
//     token = getToken();
//   }

//   free(token);
//   closeInputStream();
//   return IO_SUCCESS;
// }

/******************************************************************/

// int main(int argc, char *argv[]) {
//   if (argc <= 1) {
//     printf("scanner: no input file.\n");
//     return -1;
//   }

//   if (scan(argv[1]) == IO_ERROR) {
//     printf("Can\'t read input file!\n");
//     return -1;
//   }
    
//   return 0;
// }



