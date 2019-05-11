//
// Created by David Clements on 2019-05-11.
//

typedef struct Expr Expr;
typedef struct Stmt Stmt;
typedef struct Decl Decl;
typedef struct Typespec Typespec;

typedef enum TypespecKind {
    TYPESPEC_NONE,
    TYPESPEC_NAME,
    TYPESPEC_PAREN,
    TYPESPEC_FUNC,
    TYPESPEC_ARRAY,
    TYPESPEC_POINTER,
} TypespecKind;

struct Typespec {
    TypespecKind kind;
};

typedef enum DeclKind {
    DECL_NONE,
    DECL_ENUM,
    DECL_STRUCT,
    DECL_UNION,
    DECL_VAR,
    DECL_CONST,
    DECL_TYPEDEF,
    DECL_FUNC,
} DeclKind;

typedef struct EnumItem {
    const char *name;
    Typespec *type;
} EnumItem;

typedef struct AggregateItem {
    const char **names;
    Typespec *type;
} AggregateItem;

typedef struct FuncParam {
    const char *name;
    Typespec *type;
} FuncParam;

typedef struct FuncDecl {
    FuncParam *params;
    Typespec *return_type;
} FuncDecl;

struct Decl {
    DeclKind kind;
    const char *name;
    union {
        EnumItem *enum_items;
        AggregateItem *aggregate_items;
        struct {
            Typespec *type;
            Expr *expr;
        };

    };
};

typedef enum ExprKind {
    EXPR_NONE,
    EXPR_INT,
    EXPR_FLOAT,
    EXPR_STR,
    EXPR_NAME,
    EXPR_CAST,
    EXPR_CALL,
    EXPR_INDEX,
    EXPR_FIELD,
    EXPR_COMPOUND,
    EXPR_UNARY,
    EXPR_BINARY,
    EXPR_TERNARY,
} ExprKind;
struct Expr {
    ExprKind kind;
    union {
        uint64_t int_val;
        double float_val;
        const char *str_val;
        struct {
            Typespec *compound_type;
            Expr **compound_args;
        };
        const char *name;
        struct {
            //Unary
            Expr *operand;
            struct {
                const char *field;
                Expr **args;
                Expr *index;
            };
        };
        struct {
            //Binary
            Expr *left;
            Expr *right;
        };
        struct {
            //Ternary
            Expr *cond;
            Expr *if_expr;
            Expr *else_expr;
        };
    };
};

typedef enum StmtKind {
    STMT_NONE,
    STMT_RETURN,
    STMT_BREAK,
    STMT_CONTINUE,
    STMT_BLOCK,
    STMT_IF,
    STMT_WHILE,
    STMT_FOR,
    STMT_DO,
    STMT_SWITCH,
    STMT_ASSIGN,
    STMT_AUTO_ASSIGN,
    STMT_EXPR,
} StmtKind;
struct Stmt {
    StmtKind kind;
};