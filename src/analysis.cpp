#include "compiler.hpp"
#include <Tracy.hpp>

struct AnalyzerState {
    File *file;
    ace::Array<Scope *> scope_stack;
};

bool interp_expr(Compiler *compiler, ExprRef expr_ref, InterpValue *out_value)
{
    Expr expr = expr_ref.get(compiler);
    switch (expr.kind) {
    case ExprKind_IntLiteral: {
        InterpValue value = {};
        value.type_ref = compiler->untyped_int_type;
        value.i64 = expr.int_literal.i64;
        *out_value = value;
        return true;
    }
    case ExprKind_FloatLiteral: {
        InterpValue value = {};
        value.type_ref = compiler->untyped_float_type;
        value.f64 = expr.float_literal.f64;
        *out_value = value;
        return true;
    }
    default: break;
    }

    return false;
}

static void
analyze_stmt(Compiler *compiler, AnalyzerState *state, StmtRef stmt_ref);
static void
analyze_decl(Compiler *compiler, AnalyzerState *state, DeclRef decl_ref);

static void analyze_expr(
    Compiler *compiler,
    AnalyzerState *state,
    ExprRef expr_ref,
    TypeRef expected_type_ref = {0})
{
    ZoneScoped;

    ACE_ASSERT(expr_ref.id > 0);
    Expr expr = compiler->exprs[expr_ref.id];

    switch (expr.kind) {
    case ExprKind_Unknown: {
        ACE_ASSERT(0);
        break;
    }
    case ExprKind_VoidType: {
        expr.expr_type_ref = compiler->type_type;
        expr.as_type_ref = compiler->void_type;
        break;
    }
    case ExprKind_BoolType: {
        expr.expr_type_ref = compiler->type_type;
        expr.as_type_ref = compiler->bool_type;
        break;
    }
    case ExprKind_IntType: {
        expr.expr_type_ref = compiler->type_type;
        if (expr.int_type.is_signed) {
            switch (expr.int_type.bits) {
            case 8: expr.as_type_ref = compiler->i8_type; break;
            case 16: expr.as_type_ref = compiler->i16_type; break;
            case 32: expr.as_type_ref = compiler->i32_type; break;
            case 64: expr.as_type_ref = compiler->i64_type; break;
            }
        } else {
            switch (expr.int_type.bits) {
            case 8: expr.as_type_ref = compiler->u8_type; break;
            case 16: expr.as_type_ref = compiler->u16_type; break;
            case 32: expr.as_type_ref = compiler->u32_type; break;
            case 64: expr.as_type_ref = compiler->u64_type; break;
            }
        }
        break;
    }
    case ExprKind_FloatType: {
        expr.expr_type_ref = compiler->type_type;
        switch (expr.float_type.bits) {
        case 32: expr.as_type_ref = compiler->f32_type; break;
        case 64: expr.as_type_ref = compiler->f64_type; break;
        }
        break;
    }
    case ExprKind_PointerType: {
        analyze_expr(
            compiler, state, expr.ptr_type.sub_expr_ref, compiler->type_type);
        TypeRef sub_type = expr.ptr_type.sub_expr_ref.get(compiler).as_type_ref;
        if (sub_type.id) {
            expr.expr_type_ref = compiler->type_type;
            expr.as_type_ref = compiler->create_pointer_type(sub_type);
        }
        break;
    }
    case ExprKind_SliceType: {
        analyze_expr(
            compiler,
            state,
            expr.slice_type.subtype_expr_ref,
            compiler->type_type);
        TypeRef sub_type =
            expr.slice_type.subtype_expr_ref.get(compiler).as_type_ref;
        if (sub_type.id) {
            expr.expr_type_ref = compiler->type_type;
            expr.as_type_ref = compiler->create_slice_type(sub_type);
        }
        break;
    }
    case ExprKind_ArrayType: {
        analyze_expr(
            compiler,
            state,
            expr.array_type.subtype_expr_ref,
            compiler->type_type);
        analyze_expr(
            compiler,
            state,
            expr.array_type.size_expr_ref,
            compiler->untyped_int_type);

        TypeRef sub_type =
            expr.array_type.subtype_expr_ref.get(compiler).as_type_ref;
        if (sub_type.id) {
            InterpValue interp_value = {};
            if (interp_expr(
                    compiler, expr.array_type.size_expr_ref, &interp_value)) {
                ACE_ASSERT(
                    interp_value.type_ref.id == compiler->untyped_int_type.id);
                expr.expr_type_ref = compiler->type_type;
                expr.as_type_ref =
                    compiler->create_array_type(sub_type, interp_value.i64);
            } else {
                compiler->add_error(
                    expr.array_type.size_expr_ref.get(compiler).loc,
                    "array size expression does not evaluate to a compile-time "
                    "integer");
            }
        }
        break;
    }
    case ExprKind_BoolLiteral: {
        expr.expr_type_ref = compiler->bool_type;
        break;
    }
    case ExprKind_IntLiteral: {
        if (expected_type_ref.id &&
            (expected_type_ref.get(compiler).kind == TypeKind_Int ||
             expected_type_ref.get(compiler).kind == TypeKind_Float)) {
            expr.expr_type_ref = expected_type_ref;
        } else {
            expr.expr_type_ref = compiler->untyped_int_type;
        }
        break;
    }
    case ExprKind_FloatLiteral: {
        if (expected_type_ref.id &&
            expected_type_ref.get(compiler).kind == TypeKind_Float) {
            expr.expr_type_ref = expected_type_ref;
        } else {
            expr.expr_type_ref = compiler->untyped_float_type;
        }
        break;
    }
    case ExprKind_StringLiteral: {
        expr.expr_type_ref = compiler->create_slice_type(compiler->u8_type);
        if (expected_type_ref.id) {
            Type expected_type = expected_type_ref.get(compiler);
            if (expected_type.kind == TypeKind_Pointer &&
                expected_type.pointer.sub_type.id == compiler->u8_type.id) {
                expr.expr_type_ref = expected_type_ref;
            }
        }
        break;
    }
    case ExprKind_NullLiteral: {
        if (expected_type_ref.id &&
            expected_type_ref.get(compiler).kind == TypeKind_Pointer) {
            expr.expr_type_ref = expected_type_ref;
        } else {
            expr.expr_type_ref =
                compiler->create_pointer_type(compiler->void_type);
        }
        break;
    }
    case ExprKind_VoidLiteral: {
        expr.expr_type_ref = compiler->void_type;
        break;
    }
    case ExprKind_Identifier: {
        Scope *scope = *state->scope_stack.last();
        DeclRef decl_ref = scope->lookup(expr.ident.str);
        if (decl_ref.id) {
            Decl decl = decl_ref.get(compiler);
            expr.expr_type_ref = decl.decl_type_ref;
            expr.as_type_ref = decl.as_type_ref;
            expr.ident.decl_ref = decl_ref;
        } else {
            compiler->add_error(
                expr.loc,
                "identifier '%.*s' does not refer to a symbol",
                (int)expr.ident.str.len,
                expr.ident.str.ptr);
        }
        break;
    }
    case ExprKind_Function: {
        compiler->add_error(expr.loc, "unimplemented function expr");
        break;
    }
    case ExprKind_FunctionCall: {
        analyze_expr(compiler, state, expr.func_call.func_expr_ref);

        Expr func_expr = expr.func_call.func_expr_ref.get(compiler);
        Type func_type = func_expr.expr_type_ref.get(compiler);
        if (func_type.kind != TypeKind_Function) {
            compiler->add_error(
                func_expr.loc, "expected expression to have function type");
            break;
        }

        if (func_type.func.param_types.len != expr.func_call.param_refs.len) {
            compiler->add_error(
                expr.loc,
                "expected '%zu' parameters for function call, instead got "
                "'%zu'",
                func_type.func.param_types.len,
                expr.func_call.param_refs.len);
            break;
        }

        for (size_t i = 0; i < expr.func_call.param_refs.len; ++i) {
            TypeRef param_expected_type = func_type.func.param_types[i];
            analyze_expr(
                compiler,
                state,
                expr.func_call.param_refs[i],
                param_expected_type);
        }

        expr.expr_type_ref = func_type.func.return_type;

        break;
    }
    case ExprKind_Unary: {
        compiler->add_error(expr.loc, "unimplemented unary expr");
        break;
    }
    case ExprKind_Binary: {
        compiler->add_error(expr.loc, "unimplemented binary expr");
        break;
    }
    }

    if (expected_type_ref.id != 0 &&
        expected_type_ref.id != expr.expr_type_ref.id) {
        Type expected_type = expected_type_ref.get(compiler);
        Type expr_type = expr.expr_type_ref.get(compiler);

        ace::String expected_type_str = expected_type.to_string(compiler);
        ace::String expr_type_str = expr_type.to_string(compiler);

        compiler->add_error(
            expr.loc,
            "unmatched types, expecting '%.*s', but got '%.*s'",
            (int)expected_type_str.len,
            expected_type_str.ptr,
            (int)expr_type_str.len,
            expr_type_str.ptr);
    }

    compiler->exprs[expr_ref.id] = expr;
}

static void
analyze_stmt(Compiler *compiler, AnalyzerState *state, StmtRef stmt_ref)
{
    ZoneScoped;

    ACE_ASSERT(stmt_ref.id > 0);
    Stmt stmt = compiler->stmts[stmt_ref.id];

    switch (stmt.kind) {
    case StmtKind_Unknown: {
        ACE_ASSERT(0);
        break;
    }
    case StmtKind_Block: {
        compiler->add_error(stmt.loc, "unimplemented block stmt");
        break;
    }
    case StmtKind_Expr: {
        analyze_expr(compiler, state, stmt.expr.expr_ref);
        break;
    }
    case StmtKind_Return: {
        compiler->add_error(stmt.loc, "unimplemented return stmt");
        break;
    }
    case StmtKind_If: {
        compiler->add_error(stmt.loc, "unimplemented if stmt");
        break;
    }
    case StmtKind_While: {
        compiler->add_error(stmt.loc, "unimplemented while stmt");
        break;
    }
    }

    compiler->stmts[stmt_ref.id] = stmt;
}

static void
analyze_decl(Compiler *compiler, AnalyzerState *state, DeclRef decl_ref)
{
    ZoneScoped;

    ACE_ASSERT(decl_ref.id > 0);
    Decl decl = compiler->decls[decl_ref.id];

    switch (decl.kind) {
    case DeclKind_Unknown: {
        ACE_ASSERT(0);
        break;
    }
    case DeclKind_ConstDecl: {
        compiler->add_error(decl.loc, "const decl unimplemented");
        break;
    }
    case DeclKind_Function: {
        decl.func.scope =
            Scope::create(compiler, state->file, *state->scope_stack.last());

        for (ExprRef return_type_expr_ref : decl.func.return_type_expr_refs) {
            analyze_expr(
                compiler, state, return_type_expr_ref, compiler->type_type);
        }

        TypeRef return_type = {};
        if (decl.func.return_type_expr_refs.len == 0) {
            return_type = compiler->void_type;
        } else if (decl.func.return_type_expr_refs.len == 1) {
            return_type =
                decl.func.return_type_expr_refs[0].get(compiler).as_type_ref;
        } else {
            ace::Slice<TypeRef> fields = compiler->arena->alloc<TypeRef>(
                decl.func.return_type_expr_refs.len);

            for (size_t i = 0; i < decl.func.return_type_expr_refs.len; ++i) {
                fields[i] = decl.func.return_type_expr_refs[i]
                                .get(compiler)
                                .as_type_ref;
            }

            return_type = compiler->create_tuple_type(fields);
        }

        ace::Slice<TypeRef> param_types =
            compiler->arena->alloc<TypeRef>(decl.func.param_decl_refs.len);

        for (size_t i = 0; i < decl.func.param_decl_refs.len; ++i) {
            DeclRef param_decl_ref = decl.func.param_decl_refs[i];
            analyze_decl(compiler, state, param_decl_ref);
            param_types[i] = param_decl_ref.get(compiler).decl_type_ref;

            decl.func.scope->add(compiler, param_decl_ref);
        }

        decl.decl_type_ref =
            compiler->create_func_type(return_type, param_types);

        state->scope_stack.push_back(decl.func.scope);
        for (StmtRef stmt_ref : decl.func.body_stmts) {
            analyze_stmt(compiler, state, stmt_ref);
        }
        state->scope_stack.pop();

        break;
    }
    case DeclKind_FunctionParameter: {
        analyze_expr(
            compiler, state, decl.func_param.type_expr, compiler->type_type);
        decl.decl_type_ref =
            decl.func_param.type_expr.get(compiler).as_type_ref;
        break;
    }
    case DeclKind_LocalVarDecl: {
        compiler->add_error(decl.loc, "local var decl unimplemented");
        break;
    }
    case DeclKind_GlobalVarDecl: {
        compiler->add_error(decl.loc, "global var decl unimplemented");
        break;
    }
    }

    compiler->decls[decl_ref.id] = decl;
}

void analyze_file(Compiler *compiler, File *file)
{
    ZoneScoped;

    AnalyzerState state = {};
    state.file = file;
    state.scope_stack = ace::Array<Scope *>::create(compiler->arena);

    state.scope_stack.push_back(file->scope);

    // Register top level symbols
    for (DeclRef decl_ref : file->top_level_decls) {
        file->scope->add(compiler, decl_ref);
    }

    for (DeclRef decl_ref : state.file->top_level_decls) {
        analyze_decl(compiler, &state, decl_ref);
    }

    state.scope_stack.pop();

    ACE_ASSERT(state.scope_stack.len == 0);

    if (compiler->errors.len > 0) {
        compiler->halt_compilation();
    }
}