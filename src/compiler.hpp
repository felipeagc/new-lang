#pragma once

#include "base.hpp"

struct Compiler;
struct File;

struct Expr;
struct Stmt;
struct Decl;

struct Type;

struct FileRef {
    uint32_t id;
};

struct DeclRef {
    uint32_t id;

    Decl get(Compiler *compiler) const;
};

struct StmtRef {
    uint32_t id;

    Stmt get(Compiler *compiler) const;
};

struct ExprRef {
    uint32_t id;

    Expr get(Compiler *compiler) const;
    bool is_lvalue(Compiler *compiler);
};

struct TypeRef {
    uint32_t id;

    Type get(Compiler *compiler) const;
    bool is_runtime(Compiler *compiler);
};

struct Scope {
    FileRef file_ref;
    Scope *parent;
    StringMap<DeclRef> decl_refs;

    static Scope *
    create(Compiler *compiler, FileRef file_ref, Scope *parent = nullptr);
    void add(Compiler *compiler, DeclRef decl_ref);
    DeclRef lookup(const String &name);
};

struct File {
    String path;
    String text;
    size_t line_count;

    Scope *scope;
    Array<DeclRef> top_level_decls;
};

struct Location {
    FileRef file_ref;
    uint32_t offset;
    uint32_t len;
    uint32_t line;
    uint32_t col;
};

struct Error {
    Location loc;
    String message;
};

enum TokenKind {
    TokenKind_Unknown = 0,
    TokenKind_Error,

    TokenKind_LParen,
    TokenKind_RParen,
    TokenKind_LBracket,
    TokenKind_RBracket,
    TokenKind_LCurly,
    TokenKind_RCurly,

    TokenKind_Colon,
    TokenKind_Comma,
    TokenKind_Dot,
    TokenKind_Semicolon,
    TokenKind_Question,

    TokenKind_Equal,

    TokenKind_Add,
    TokenKind_Sub,
    TokenKind_Mul,
    TokenKind_Div,
    TokenKind_Mod,
    TokenKind_Arrow,

    TokenKind_Not,
    TokenKind_BitAnd,
    TokenKind_BitOr,
    TokenKind_BitXor,
    TokenKind_BitNot,

    TokenKind_EqualEqual,
    TokenKind_NotEqual,
    TokenKind_Less,
    TokenKind_LessEqual,
    TokenKind_Greater,
    TokenKind_GreaterEqual,

    TokenKind_LShift,
    TokenKind_RShift,

    TokenKind_AddEqual,
    TokenKind_SubEqual,
    TokenKind_MulEqual,
    TokenKind_DivEqual,
    TokenKind_ModEqual,

    TokenKind_BitAndEqual,
    TokenKind_BitOrEqual,
    TokenKind_BitXorEqual,
    TokenKind_BitNotEqual,
    TokenKind_LShiftEqual,
    TokenKind_RShiftEqual,

    TokenKind_Global,
    TokenKind_Const,
    TokenKind_Extern,
    TokenKind_Export,
    TokenKind_Inline,
    TokenKind_VarArg,
    TokenKind_Def,
    TokenKind_Macro,
    TokenKind_Type,
    TokenKind_Struct,
    TokenKind_Union,
    TokenKind_If,
    TokenKind_Else,
    TokenKind_While,
    TokenKind_Break,
    TokenKind_Continue,
    TokenKind_Return,
    TokenKind_And,
    TokenKind_Or,
    TokenKind_Void,
    TokenKind_Bool,
    TokenKind_True,
    TokenKind_False,
    TokenKind_Null,
    TokenKind_U8,
    TokenKind_U16,
    TokenKind_U32,
    TokenKind_U64,
    TokenKind_I8,
    TokenKind_I16,
    TokenKind_I32,
    TokenKind_I64,
    TokenKind_F32,
    TokenKind_F64,
    TokenKind_Identifier,
    TokenKind_BuiltinIdentifier,
    TokenKind_StringLiteral,
    TokenKind_CharLiteral,
    TokenKind_IntLiteral,
    TokenKind_FloatLiteral,

    TokenKind_EOF,
};

struct Token {
    TokenKind kind;
    Location loc;
    union {
        String str;
        int64_t int_;
        double float_;
    };
};

enum BuiltinFunction {
    BuiltinFunction_Unknown = 0,
    BuiltinFunction_Sizeof,
    BuiltinFunction_Alignof,
    BuiltinFunction_PtrCast,
};

enum TypeKind {
    TypeKind_Unknown = 0,
    TypeKind_Void,
    TypeKind_Type,
    TypeKind_Bool,
    TypeKind_UntypedInt,
    TypeKind_UntypedFloat,
    TypeKind_Int,
    TypeKind_Float,
    TypeKind_Struct,
    TypeKind_Tuple,
    TypeKind_Pointer,
    TypeKind_Array,
    TypeKind_Slice,
    TypeKind_Function,
};

struct Type {
    TypeKind kind;
    String str;
    union {
        struct {
            uint32_t bits;
            bool is_signed;
        } int_;
        struct {
            uint32_t bits;
        } float_;
        struct {
            Slice<TypeRef> field_types;
            Slice<String> field_names;
            StringMap<uint32_t> field_map;
        } struct_;
        struct {
            Slice<TypeRef> field_types;
        } tuple;
        struct {
            TypeRef sub_type;
        } pointer;
        struct {
            TypeRef sub_type;
            uint64_t size;
        } array;
        struct {
            TypeRef sub_type;
        } slice;
        struct {
            TypeRef return_type;
            Slice<TypeRef> param_types;
            bool vararg;
        } func;
    };

    String to_string(Compiler *compiler);
    uint32_t align_of(Compiler *compiler);
    uint32_t size_of(Compiler *compiler);
};

struct InterpValue {
    TypeRef type_ref;
    union {
        uint8_t u8;
        uint16_t u16;
        uint32_t u32;
        uint64_t u64;
        int8_t i8;
        int16_t i16;
        int32_t i32;
        int64_t i64;
        float f32;
        double f64;
    };
};

enum FunctionFlags {
    FunctionFlags_Inline = 1 << 0,
    FunctionFlags_Extern = 1 << 1,
    FunctionFlags_Exported = 1 << 2,
    FunctionFlags_VarArg = 1 << 3,
};

enum UnaryOp {
    UnaryOp_Unknown = 0,

    UnaryOp_AddressOf,
    UnaryOp_Dereference,
    UnaryOp_Not,
    UnaryOp_Negate,
};

enum BinaryOp {
    BinaryOp_Unknown = 0,

    BinaryOp_Add,
    BinaryOp_Sub,
    BinaryOp_Mul,
    BinaryOp_Div,
    BinaryOp_Mod,
    BinaryOp_BitAnd,
    BinaryOp_BitOr,
    BinaryOp_BitXor,
    BinaryOp_LShift,
    BinaryOp_RShift,
    BinaryOp_Equal,
    BinaryOp_NotEqual,
    BinaryOp_Less,
    BinaryOp_LessEqual,
    BinaryOp_Greater,
    BinaryOp_GreaterEqual,
    BinaryOp_MAX,
};

enum ExprKind {
    ExprKind_Unknown = 0,
    ExprKind_Identifier,
    ExprKind_StringLiteral,
    ExprKind_IntLiteral,
    ExprKind_FloatLiteral,
    ExprKind_BoolLiteral,
    ExprKind_VoidLiteral,
    ExprKind_Function,
    ExprKind_FunctionCall,
    ExprKind_NullLiteral,
    ExprKind_PointerType,
    ExprKind_VoidType,
    ExprKind_BoolType,
    ExprKind_IntType,
    ExprKind_FloatType,
    ExprKind_SliceType,
    ExprKind_ArrayType,
    ExprKind_StructType,
    ExprKind_Subscript,
    ExprKind_Access,
    ExprKind_BuiltinCall,
    ExprKind_Unary,
    ExprKind_Binary,
};

struct Expr {
    ExprKind kind;
    TypeRef expr_type_ref;
    TypeRef as_type_ref;
    Location loc;
    union {
        struct {
            String str;
            DeclRef decl_ref;
        } ident;
        struct {
            String str;
        } str_literal;
        struct {
            int64_t i64;
        } int_literal;
        struct {
            double f64;
        } float_literal;
        struct {
            bool bool_;
        } bool_literal;
        struct {
            uint32_t flags;
            Array<ExprRef> return_type_expr_refs;
            Array<DeclRef> param_decl_refs;
            Array<StmtRef> body_stmts;
        } func;
        struct {
            ExprRef func_expr_ref;
            Array<ExprRef> param_refs;
        } func_call;
        struct {
            BuiltinFunction builtin;
            Array<ExprRef> param_refs;
        } builtin_call;
        struct {
            ExprRef sub_expr_ref;
        } ptr_type;
        struct {
            uint32_t bits;
            bool is_signed;
        } int_type;
        struct {
            uint32_t bits;
        } float_type;
        struct {
            ExprRef subtype_expr_ref;
        } slice_type;
        struct {
            ExprRef subtype_expr_ref;
            ExprRef size_expr_ref;
        } array_type;
        struct {
            Array<String> field_names;
            Array<ExprRef> field_type_expr_refs;
        } struct_type;
        struct {
            ExprRef left_ref;
            ExprRef right_ref;
        } subscript;
        struct {
            ExprRef left_ref;
            ExprRef accessed_ident_ref;
        } access;
        struct {
            ExprRef left_ref;
            ExprRef right_ref;
            BinaryOp op;
        } binary;
        struct {
            ExprRef left_ref;
            UnaryOp op;
        } unary;
    };
};

enum StmtKind {
    StmtKind_Unknown = 0,
    StmtKind_Block,
    StmtKind_Expr,
    StmtKind_Decl,
    StmtKind_If,
    StmtKind_While,
    StmtKind_Return,
    StmtKind_Assign,
};

struct Stmt {
    StmtKind kind;
    Location loc;
    union {
        struct {
            Array<StmtRef> stmt_refs;
        } block;
        struct {
            ExprRef expr_ref;
        } expr;
        struct {
            DeclRef decl_ref;
        } decl;
        struct {
            ExprRef returned_expr_ref;
        } return_;
        struct {
            ExprRef cond_expr_ref;
            StmtRef true_stmt_ref;
            StmtRef false_stmt_ref;
        } if_;
        struct {
            ExprRef cond_expr_ref;
            StmtRef true_stmt_ref;
        } while_;
        struct {
            ExprRef assigned_expr_ref;
            ExprRef value_expr_ref;
        } assign;
    };
};

enum DeclKind {
    DeclKind_Unknown = 0,
    DeclKind_Type,
    DeclKind_Function,
    DeclKind_FunctionParameter,
    DeclKind_LocalVarDecl,
    DeclKind_GlobalVarDecl,
    DeclKind_ConstDecl,
};

struct Decl {
    DeclKind kind;
    TypeRef decl_type_ref;
    TypeRef as_type_ref;
    Location loc;
    String name;
    union {
        struct {
            Scope *scope;
            uint32_t flags;
            Array<ExprRef> return_type_expr_refs;
            Array<DeclRef> param_decl_refs;
            Array<StmtRef> body_stmts;
        } func;
        struct {
            ExprRef type_expr;
        } func_param;
        struct {
            ExprRef type_expr;
            ExprRef value_expr;
        } local_var_decl;
        struct {
            ExprRef type_expr;
            ExprRef value_expr;
        } global_var_decl;
        struct {
            ExprRef type_expr;
            ExprRef value_expr;
        } const_decl;
        struct {
            ExprRef type_expr;
        } type_decl;
    };
};

struct Compiler {
    ArenaAllocator *arena;
    StringMap<TokenKind> keyword_map;
    StringMap<BuiltinFunction> builtin_function_map;
    Array<Error> errors;
    StringBuilder sb;

    Array<File> files;
    StringMap<TypeRef> type_map;
    Array<Type> types;
    Array<Decl> decls;
    Array<Stmt> stmts;
    Array<Expr> exprs;

    TypeRef void_type;
    TypeRef type_type;
    TypeRef bool_type;
    TypeRef untyped_int_type;
    TypeRef untyped_float_type;
    TypeRef u8_type;
    TypeRef u16_type;
    TypeRef u32_type;
    TypeRef u64_type;
    TypeRef i8_type;
    TypeRef i16_type;
    TypeRef i32_type;
    TypeRef i64_type;
    TypeRef f32_type;
    TypeRef f64_type;

    static Compiler create();
    void destroy();

    TypeRef get_cached_type(Type &type);
    TypeRef create_pointer_type(TypeRef sub);
    TypeRef
    create_struct_type(Slice<TypeRef> fields, Slice<String> field_names);
    TypeRef create_tuple_type(Slice<TypeRef> fields);
    TypeRef create_array_type(TypeRef sub, size_t size);
    TypeRef create_slice_type(TypeRef sub);
    TypeRef create_func_type(
        TypeRef return_type, Slice<TypeRef> param_types, bool vararg);

    size_t get_error_checkpoint();
    void restore_error_checkpoint(size_t checkpoint);
    LANG_PRINTF_FORMATTING(3, 4)
    void add_error(const Location &loc, const char *fmt, ...);
    void halt_compilation();
    void print_errors();

    LANG_INLINE
    FileRef add_file(const File &file)
    {
        FileRef ref = {(uint32_t)this->files.len};
        this->files.push_back(file);
        return ref;
    }

    LANG_INLINE
    ExprRef add_expr(const Expr &expr)
    {
        LANG_ASSERT(expr.kind != ExprKind_Unknown);
        ExprRef ref = {(uint32_t)this->exprs.len};
        this->exprs.push_back(expr);
        return ref;
    }

    LANG_INLINE
    StmtRef add_stmt(const Stmt &stmt)
    {
        LANG_ASSERT(stmt.kind != StmtKind_Unknown);
        StmtRef ref = {(uint32_t)this->stmts.len};
        this->stmts.push_back(stmt);
        return ref;
    }

    LANG_INLINE
    DeclRef add_decl(const Decl &decl)
    {
        LANG_ASSERT(decl.kind != DeclKind_Unknown);
        DeclRef ref = {(uint32_t)this->decls.len};
        this->decls.push_back(decl);
        return ref;
    }

    LANG_INLINE
    Expr *get_expr(ExprRef ref)
    {
        return &this->exprs[ref.id];
    }

    LANG_INLINE
    Stmt *get_stmt(StmtRef ref)
    {
        return &this->stmts[ref.id];
    }

    LANG_INLINE
    Decl *get_decl(DeclRef ref)
    {
        return &this->decls[ref.id];
    }

    void compile(String path);
};

void init_parser_tables();
void parse_file(Compiler *compiler, FileRef file_ref);
void analyze_file(Compiler *compiler, FileRef file_ref);
void codegen_file(Compiler *compiler, FileRef file_ref);

inline Decl DeclRef::get(Compiler *compiler) const
{
    return compiler->decls[this->id];
}

inline Stmt StmtRef::get(Compiler *compiler) const
{
    return compiler->stmts[this->id];
}

inline Expr ExprRef::get(Compiler *compiler) const
{
    return compiler->exprs[this->id];
}

inline Type TypeRef::get(Compiler *compiler) const
{
    return compiler->types[this->id];
}
