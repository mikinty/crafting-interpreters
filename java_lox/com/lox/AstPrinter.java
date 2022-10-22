package java_lox.com.lox;

import java_lox.com.lox.Expr.Assign;
import java_lox.com.lox.Expr.Call;
import java_lox.com.lox.Expr.Get;
import java_lox.com.lox.Expr.Logical;
import java_lox.com.lox.Expr.Set;
import java_lox.com.lox.Expr.This;
import java_lox.com.lox.Expr.Variable;

class AstPrinter implements Expr.Visitor<String> {
    String print(Expr expr) {
        return expr.accept(this);
    }

    @Override 
    public String visitBinaryExpr(Expr.Binary expr) {
        return parenthesize(expr.operator.lexeme, expr.left, expr.right);
    }

    @Override 
    public String visitGroupingExpr(Expr.Grouping expr) {
        return parenthesize("group", expr.expression);
    }

    @Override 
    public String visitLiteralExpr(Expr.Literal expr) {
        if (expr.value == null) return "nil";
        return expr.value.toString();
    }

    @Override 
    public String visitUnaryExpr(Expr.Unary expr) {
        return parenthesize(expr.operator.lexeme, expr.right);
    }

    private String parenthesize(String name, Expr... exprs) {
        StringBuilder builder = new StringBuilder();

        builder.append("(").append(name);
        for (Expr expr : exprs) {
            builder.append(" ");
            builder.append(expr.accept(this));
        }
        builder.append(")");

        return builder.toString();
    }

    @Override
    public String visitVariableExpr(Variable expr) {
        return "var " + expr.name.lexeme;
    }

    @Override
    public String visitAssignExpr(Assign expr) {
        return "assign var " + expr.name.lexeme;
    }

    @Override
    public String visitLogicalExpr(Logical expr) {
        // TODO Auto-generated method stub
        return null;
    }

    @Override
    public String visitCallExpr(Call expr) {
        // TODO Auto-generated method stub
        return null;
    }

    @Override
    public String visitGetExpr(Get expr) {
        // TODO Auto-generated method stub
        return null;
    }

    @Override
    public String visitSetExpr(Set expr) {
        // TODO Auto-generated method stub
        return null;
    }

    @Override
    public String visitThisExpr(This expr) {
        // TODO Auto-generated method stub
        return null;
    }
}
