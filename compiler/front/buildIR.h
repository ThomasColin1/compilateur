#pragma once

#include "antlr4-runtime.h"
#include "../generated/ifccBaseVisitor.h"

#include "../back/IR.h"
#include <list>
#include <string>

class buildIR : public ifccBaseVisitor
{
public:
	virtual antlrcpp::Any visitProg(ifccParser::ProgContext *ctx) override;
	virtual antlrcpp::Any visitFunction(ifccParser::FunctionContext *ctx) override;
	virtual antlrcpp::Any visitStatement(ifccParser::StatementContext *ctx) override;
	virtual antlrcpp::Any visitDeclaration(ifccParser::DeclarationContext *ctx) override;
	virtual antlrcpp::Any visitDefinition(ifccParser::DefinitionContext *ctx) override;
	virtual antlrcpp::Any visitDeclpartg(ifccParser::DeclpartgContext *ctx) override;
	virtual antlrcpp::Any visitVarpartg(ifccParser::VarpartgContext *ctx) override;
	virtual antlrcpp::Any visitRetour(ifccParser::RetourContext *context) override;
	virtual antlrcpp::Any visitAddsub(ifccParser::AddsubContext *context) override;
	virtual antlrcpp::Any visitConstExpr(ifccParser::ConstExprContext *ctx) override;
	virtual antlrcpp::Any visitVarExpr(ifccParser::VarExprContext *ctx) override;
	virtual antlrcpp::Any visitMultdiv(ifccParser::MultdivContext *ctx) override;
	virtual antlrcpp::Any visitPar(ifccParser::ParContext *ctx) override;
	virtual antlrcpp::Any visitBoolDiffEgal(ifccParser::BoolDiffEgalContext *ctx) override;
	virtual antlrcpp::Any visitBoolInfSup(ifccParser::BoolInfSupContext *ctx) override;
	virtual antlrcpp::Any visitUnaireNegNot(ifccParser::UnaireNegNotContext *ctx) override;
	virtual antlrcpp::Any visitExprFunctionCall(ifccParser::ExprFunctionCallContext *ctx) override;
	virtual antlrcpp::Any visitFunctionCall(ifccParser::FunctionCallContext *ctx) override;
	virtual antlrcpp::Any visitBlock(ifccParser::BlockContext *ctx) override;
	virtual antlrcpp::Any visitBlockif(ifccParser::BlockifContext *ctx) override;
	virtual antlrcpp::Any visitBlockwhile(ifccParser::BlockwhileContext *ctx) override;
private:
	list<CFG*>* cfgs;
	map<string, int> functionTable;
	CFG* currentCFG;
	int countBlock;
};
