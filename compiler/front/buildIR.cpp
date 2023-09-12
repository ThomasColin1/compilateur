#include "buildIR.h"

antlrcpp::Any buildIR::visitProg(ifccParser::ProgContext *ctx)
{
	// Initialisation du vecteur de CFG
	cfgs = new list<CFG *>();

	// Initialisation de la table des fonctions
	functionTable = map<string, int>();
	if (ctx->function().size())
	{
		for (int i = 0; i < ctx->function().size(); i++)
		{
			string label = ctx->function()[i]->VARNAME()->getText();
			int nbParams = 0;
			if (ctx->function()[i]->params())
			{
				nbParams = ctx->function()[i]->params()->VARNAME().size();
				if (nbParams > 6)
				{
					cerr << "Notre compilateur ne gère pas les fonctions avec plus de 6 paramètres." << endl;
					exit(1);
				}
			}
			if (functionTable.find(label) == functionTable.end())
			{
				functionTable.insert(pair<string, int>(label, nbParams));
			}
			else
			{
				cerr << "Redéfinition de la fonction " << label << endl;
				exit(1);
			}
		}
	}

	// On lance la visite du programme
	visitChildren(ctx);

	return cfgs;
}

antlrcpp::Any buildIR::visitFunction(ifccParser::FunctionContext *ctx)
{
	// On crée un CFG pour chaque fonction
	CFG *cfg = new CFG();
	cfg->label = ctx->VARNAME()->getText();
	countBlock = 1;
	cfg->currentScope = countBlock;

	// On crée un basic block pour le prologue, le corps de la fonction et l'épilogue
	BasicBlock *prologue = new BasicBlock(cfg, "prologue", cfg->currentScope);
	BasicBlock *bb = new BasicBlock(cfg, ctx->VARNAME()->getText(), cfg->currentScope);
	BasicBlock *epilogue = new BasicBlock(cfg, "epilogue", cfg->currentScope);

	prologue->exit_true = bb;
	prologue->exit_false = nullptr;

	bb->exit_true = epilogue;
	bb->exit_false = nullptr;

	epilogue->exit_true = nullptr;
	epilogue->exit_false = nullptr;

	cfg->add_bb(prologue);
	cfg->add_bb(bb);
	cfg->add_bb(epilogue);

	// On mets à jour le pointeur sur le basic block actuel avec le BB correspondant au corps de la fonction
	cfg->current_bb = bb;
	currentCFG = cfg;

	// On crée la table des symboles associée à la portée initiale
	cfg->create_symbol_table_scope(cfg->currentScope);

	// On ajoute à la table des symboles les paramètres de la fonction
	vector<string> params = vector<string>();
	if (ctx->params())
	{
		for (int i = 0; i < ctx->params()->VARNAME().size(); i++)
		{
			string id = ctx->params()->VARNAME()[i]->getText();
			// Les paramètres d'une fonction sont forcément initialisés
			cfg->add_to_symbol_table(cfg->current_bb->scope, id, "int", true, false);
			params.push_back(id);
		}
		// On ajoute une instruction IR qui permet d'initialiser les valeurs des paramètres passées à l'appel
		bb->add_IRInstr(IRInstr::Operation::function_params_initialisation, "int", params, currentCFG->currentScope);
	}

	// On ajoute le cfg construit à la liste des CFGs du programme
	cfgs->push_back(cfg);

	// On visite l'ensemble des fonctions qui constituent le programme
	return visitChildren(ctx);
}

antlrcpp::Any buildIR::visitStatement(ifccParser::StatementContext *ctx)
{
	return visitChildren(ctx);
}

antlrcpp::Any buildIR::visitDeclaration(ifccParser::DeclarationContext *ctx)
{
	// On récupère le nombre de variables déclarées dans la déclaration
	int nbVars = ctx->VARNAME().size();
	for (int i = 0; i < nbVars; i++)
	{
		// On récupère le nom de la variable
		string id = ctx->VARNAME(i)->getText();
		// On vérifie que la variable n'a pas déjà été déclarée dans la table des symboles de la portée actuelle
		if (!currentCFG->already_defined_in_scope_symbol_table(currentCFG->current_bb->scope, id))
		{
			currentCFG->add_to_symbol_table(currentCFG->current_bb->scope, id, "int", false, false);
		}
		else
		{
			// La variable a déjà été déclarée, il faut lever une erreur de redéclaration
			currentCFG->add_error("La variable " + id + " a été redeclarée. (ligne " + to_string(ctx->getStart()->getLine()) + ")\n");
		}
	}
	return 0;
}

antlrcpp::Any buildIR::visitDefinition(ifccParser::DefinitionContext *ctx)
{
	string var1_and_scope = (string)visit(ctx->partg());
	string var1_scope = var1_and_scope.substr(0, var1_and_scope.find("-"));
	string var1 = var1_and_scope.substr(var1_and_scope.find("-") + 1);
	string var2 = (string)visit(ctx->expr());
	currentCFG->current_bb->add_IRInstr(IRInstr::Operation::copy, "int", {var1, var2, var1_scope}, currentCFG->current_bb->scope);
	return var1;
}

antlrcpp::Any buildIR::visitDeclpartg(ifccParser::DeclpartgContext *ctx)
{
	string varname = ctx->declaration()->VARNAME(0)->getText();
	if (!currentCFG->already_defined_in_scope_symbol_table(currentCFG->current_bb->scope, varname))
	{
		currentCFG->add_to_symbol_table(currentCFG->current_bb->scope, varname, "int", true, false);
		varname.insert(0, to_string(currentCFG->current_bb->scope) + "-");
	}
	else
	{
		// La variable a déjà été déclarée, il faut lever une erreur de redéclaration
		varname.insert(0, to_string(currentCFG->current_bb->scope) + "-");
		currentCFG->add_error("La variable " + varname + " a été redeclarée. (ligne " + to_string(ctx->getStart()->getLine()) + ")\n");
	}
	return varname;
}

antlrcpp::Any buildIR::visitVarpartg(ifccParser::VarpartgContext *ctx)
{
	string varname = ctx->VARNAME()->getText();
	if (currentCFG->already_defined_in_scope_symbol_table(currentCFG->current_bb->scope, varname))
	{
		// Si la variable n'a pas encore été initialisée, on l'initialise
		if (!currentCFG->get_var_is_initialized(currentCFG->current_bb->scope, varname))
		{
			currentCFG->set_var_is_initialized(currentCFG->current_bb->scope, varname);
		}
		varname.insert(0, to_string(currentCFG->current_bb->scope) + "-");
	}
	else
	{
		int scope = currentCFG->already_defined_in_another_accessible_scope(currentCFG->current_bb->scope, varname);
		if (scope)
		{
			// Si la variable n'a pas encore été initialisée, on l'initialise
			if (!currentCFG->get_var_is_initialized(scope, varname))
			{
				currentCFG->set_var_is_initialized(scope, varname);
			}
			varname.insert(0, to_string(scope) + "-");
		}
		else
		{
			// La variable en partie gauche n'est pas déclarée, on lève une erreur
			varname.insert(0, to_string(currentCFG->current_bb->scope) + "-");
			currentCFG->add_error("La variable " + varname + " n'est pas déclarée. (ligne " + to_string(ctx->getStart()->getLine()) + ")\n");
		}
	}
	return varname;
}

antlrcpp::Any buildIR::visitRetour(ifccParser::RetourContext *ctx)
{
	string var1 = (string)visit(ctx->expr());
	currentCFG->current_bb->add_IRInstr(IRInstr::Operation::ret, "int", {var1}, currentCFG->currentScope);
	return var1;
}

antlrcpp::Any buildIR::visitAddsub(ifccParser::AddsubContext *ctx)
{

	// On  récupère les cases mémoires contenant les résultats à gauche et droite de l'addition
	string var2 = (string)visit(ctx->expr()[0]);

	string var3 = (string)visit(ctx->expr()[1]);

	int scopeVar2, scopeVar3;

	// On vérifie que les variables var2 et var3 sont déclarées et initialisées, sinon on lève une erreur
	if (currentCFG->get_var_index(currentCFG->current_bb->scope, var2) != -1 && currentCFG->get_var_index(currentCFG->current_bb->scope, var3) != -1)
	{
		if (!(currentCFG->get_var_is_initialized(currentCFG->current_bb->scope, var2)))
		{
			currentCFG->add_error("La variable " + var2 + " n'est pas initialisée. (ligne " + to_string(ctx->getStart()->getLine()) + ")\n");
		}
		else if (!currentCFG->get_var_is_initialized(currentCFG->current_bb->scope, var3))
		{
			currentCFG->add_error("La variable " + var3 + " n'est pas initialisée. (ligne " + to_string(ctx->getStart()->getLine()) + ")\n");
		}
		else
		{
			// On récupère la portée de chacune des opérandes
			if (currentCFG->already_defined_in_scope_symbol_table(currentCFG->current_bb->scope, var2))
			{
				scopeVar2 = currentCFG->current_bb->scope;
			}
			else
			{
				scopeVar2 = currentCFG->already_defined_in_another_accessible_scope(currentCFG->current_bb->scope, var2);
			}

			if (currentCFG->already_defined_in_scope_symbol_table(currentCFG->current_bb->scope, var3))
			{
				scopeVar3 = currentCFG->current_bb->scope;
			}
			else
			{
				scopeVar3 = currentCFG->already_defined_in_another_accessible_scope(currentCFG->current_bb->scope, var3);
			}
		}
	}
	else
	{
		if (currentCFG->get_var_index(currentCFG->current_bb->scope, var2) != -1)
		{
			currentCFG->add_error("La variable " + var2 + " n'est pas déclarée. (ligne " + to_string(ctx->getStart()->getLine()) + ")\n");
		}
		else
		{
			currentCFG->add_error("La variable " + var3 + " n'est pas déclarée. (ligne " + to_string(ctx->getStart()->getLine()) + ")\n");
		}
	}

	// On crée une variable temporaire qui stockera le résultat de l'addition ou de la soustraction
	string var1 = currentCFG->create_new_tempvar(currentCFG->current_bb->scope, "tmp");

	// On récupère le caractère qui correspond à l'opérateur
	string total = ctx->getText();
	char operateur = total[ctx->expr()[0]->getText().length()];

	// On ajoute à la liste des instructions du BasicBlock courant l'expression IR correspondant à l'opération
	if (operateur == '+')
	{
		currentCFG->current_bb->add_IRInstr(IRInstr::Operation::add, "int", {var1, var2, var3, to_string(scopeVar2), to_string(scopeVar3)}, currentCFG->currentScope);
	}
	else if (operateur == '-')
	{
		currentCFG->current_bb->add_IRInstr(IRInstr::Operation::sub, "int", {var1, var2, var3, to_string(scopeVar2), to_string(scopeVar3)}, currentCFG->currentScope);
	}

	return var1;
}

antlrcpp::Any buildIR::visitMultdiv(ifccParser::MultdivContext *ctx)
{
	// On  récupère les cases mémoires contenant les résultats à gauche et droite de l'addition
	string var2 = (string)visit(ctx->expr()[0]);

	string var3 = (string)visit(ctx->expr()[1]);

	int scopeVar2, scopeVar3;

	// On vérifie que les variables var2 et var3 sont déclarées et initialisées, sinon on lève une erreur
	if (currentCFG->get_var_index(currentCFG->current_bb->scope, var2) != -1 && currentCFG->get_var_index(currentCFG->current_bb->scope, var3) != -1)
	{
		if (!(currentCFG->get_var_is_initialized(currentCFG->current_bb->scope, var2)))
		{
			currentCFG->add_error("La variable " + var2 + " n'est pas initialisée. (ligne " + to_string(ctx->getStart()->getLine()) + ")\n");
		}
		else if (!currentCFG->get_var_is_initialized(currentCFG->current_bb->scope, var3))
		{
			currentCFG->add_error("La variable " + var3 + " n'est pas initialisée. (ligne " + to_string(ctx->getStart()->getLine()) + ")\n");
		}
		else
		{
			// On récupère la portée de chacune des opérandes
			if (currentCFG->already_defined_in_scope_symbol_table(currentCFG->current_bb->scope, var2))
			{
				scopeVar2 = currentCFG->current_bb->scope;
			}
			else
			{
				scopeVar2 = currentCFG->already_defined_in_another_accessible_scope(currentCFG->current_bb->scope, var2);
			}

			if (currentCFG->already_defined_in_scope_symbol_table(currentCFG->current_bb->scope, var3))
			{
				scopeVar3 = currentCFG->current_bb->scope;
			}
			else
			{
				scopeVar3 = currentCFG->already_defined_in_another_accessible_scope(currentCFG->current_bb->scope, var3);
			}
		}
	}
	else
	{
		if (currentCFG->get_var_index(currentCFG->current_bb->scope, var2) != -1)
		{
			currentCFG->add_error("La variable " + var2 + " n'est pas déclarée. (ligne " + to_string(ctx->getStart()->getLine()) + ")\n");
		}
		else
		{
			currentCFG->add_error("La variable " + var3 + " n'est pas déclarée. (ligne " + to_string(ctx->getStart()->getLine()) + ")\n");
		}
	}

	// On crée une variable temporaire qui stockera le résultat de l'addition ou de la soustraction
	string var1 = currentCFG->create_new_tempvar(currentCFG->current_bb->scope, "tmp");

	// On récupère le caractère qui correspond à l'opérateur
	string total = ctx->getText();
	char operateur = total[ctx->expr()[0]->getText().length()];

	// On ajoute à la liste des instructions du BasicBlock courant l'expression IR correspondant à l'opération
	if (operateur == '*')
	{
		currentCFG->current_bb->add_IRInstr(IRInstr::Operation::mul, "int", {var1, var2, var3, to_string(scopeVar2), to_string(scopeVar3)}, currentCFG->currentScope);
	}
	else if (operateur == '/')
	{
		currentCFG->current_bb->add_IRInstr(IRInstr::Operation::div, "int", {var1, var2, var3, to_string(scopeVar2), to_string(scopeVar3)}, currentCFG->currentScope);
	}

	return var1;
}

antlrcpp::Any buildIR::visitConstExpr(ifccParser::ConstExprContext *ctx)
{
	string constante = ctx->CONST()->getText();
	string nomTmp = currentCFG->create_new_tempvar(currentCFG->current_bb->scope, "tmp");
	currentCFG->current_bb->add_IRInstr(IRInstr::Operation::ldconst, "int", {nomTmp, constante}, currentCFG->currentScope);
	return nomTmp;
}

antlrcpp::Any buildIR::visitVarExpr(ifccParser::VarExprContext *ctx)
{
	return ctx->VARNAME()->getText();
}

antlrcpp::Any buildIR::visitPar(ifccParser::ParContext *ctx)
{
	// On met la variable dans une case mémoire et on retourne cette case
	return (string)visit(ctx->expr());
}

antlrcpp::Any buildIR::visitBoolDiffEgal(ifccParser::BoolDiffEgalContext *ctx)
{
	string var2 = (string)visit(ctx->expr()[0]);

	string var3 = (string)visit(ctx->expr()[1]);

	int scopeVar2, scopeVar3;

	// On vérifie que les variables var2 et var3 sont déclarées et initialisées, sinon on lève une erreur
	if (currentCFG->get_var_index(currentCFG->current_bb->scope, var2) != -1 && currentCFG->get_var_index(currentCFG->current_bb->scope, var3) != -1)
	{
		if (!(currentCFG->get_var_is_initialized(currentCFG->current_bb->scope, var2)))
		{
			currentCFG->add_error("La variable " + var2 + " n'est pas initialisée. (ligne " + to_string(ctx->getStart()->getLine()) + ")\n");
		}
		else if (!currentCFG->get_var_is_initialized(currentCFG->current_bb->scope, var3))
		{
			currentCFG->add_error("La variable " + var3 + " n'est pas initialisée. (ligne " + to_string(ctx->getStart()->getLine()) + ")\n");
		}
		else
		{
			// On récupère la portée de chacune des opérandes
			if (currentCFG->already_defined_in_scope_symbol_table(currentCFG->current_bb->scope, var2))
			{
				scopeVar2 = currentCFG->current_bb->scope;
			}
			else
			{
				scopeVar2 = currentCFG->already_defined_in_another_accessible_scope(currentCFG->current_bb->scope, var2);
			}

			if (currentCFG->already_defined_in_scope_symbol_table(currentCFG->current_bb->scope, var3))
			{
				scopeVar3 = currentCFG->current_bb->scope;
			}
			else
			{
				scopeVar3 = currentCFG->already_defined_in_another_accessible_scope(currentCFG->current_bb->scope, var3);
			}
		}
	}
	else
	{
		if (currentCFG->get_var_index(currentCFG->current_bb->scope, var2) != -1)
		{
			currentCFG->add_error("La variable " + var2 + " n'est pas déclarée. (ligne " + to_string(ctx->getStart()->getLine()) + ")\n");
		}
		else
		{
			currentCFG->add_error("La variable " + var3 + " n'est pas déclarée. (ligne " + to_string(ctx->getStart()->getLine()) + ")\n");
		}
	}

	string var1 = currentCFG->create_new_tempvar(currentCFG->current_bb->scope, "tmp");

	string total = ctx->getText();
	char operateur = total[ctx->expr()[0]->getText().length()];

	if (operateur == '=')
	{
		currentCFG->current_bb->add_IRInstr(IRInstr::Operation::cmp_eq, "int", {var1, var2, var3, to_string(scopeVar2), to_string(scopeVar3)}, currentCFG->currentScope);
		return var1;
	}
	else
	{
		currentCFG->current_bb->add_IRInstr(IRInstr::Operation::cmp_eq, "int", {var1, var2, var3, to_string(scopeVar2), to_string(scopeVar3)}, currentCFG->currentScope);

		string var4 = currentCFG->create_new_tempvar(currentCFG->current_bb->scope, "tmp");

		currentCFG->current_bb->add_IRInstr(IRInstr::Operation::ldconst, "int", {var4, "0"}, currentCFG->currentScope);

		string var5 = currentCFG->create_new_tempvar(currentCFG->current_bb->scope, "tmp");

		currentCFG->current_bb->add_IRInstr(IRInstr::Operation::cmp_eq, "int", {var5, var1, var4, to_string(currentCFG->currentScope), to_string(currentCFG->currentScope)}, currentCFG->currentScope);

		return var5;
	}
}

antlrcpp::Any buildIR::visitBoolInfSup(ifccParser::BoolInfSupContext *ctx)
{
	string var2 = (string)visit(ctx->expr()[0]);
	string var3 = (string)visit(ctx->expr()[1]);

	int scopeVar2, scopeVar3;

	// On vérifie que les variables var2 et var3 sont déclarées et initialisées, sinon on lève une erreur
	if (currentCFG->get_var_index(currentCFG->current_bb->scope, var2) != -1 && currentCFG->get_var_index(currentCFG->current_bb->scope, var3) != -1)
	{
		if (!(currentCFG->get_var_is_initialized(currentCFG->current_bb->scope, var2)))
		{
			currentCFG->add_error("La variable " + var2 + " n'est pas initialisée. (ligne " + to_string(ctx->getStart()->getLine()) + ")\n");
		}
		else if (!currentCFG->get_var_is_initialized(currentCFG->current_bb->scope, var3))
		{
			currentCFG->add_error("La variable " + var3 + " n'est pas initialisée. (ligne " + to_string(ctx->getStart()->getLine()) + ")\n");
		}
		else
		{
			// On récupère la portée de chacune des opérandes
			if (currentCFG->already_defined_in_scope_symbol_table(currentCFG->current_bb->scope, var2))
			{
				scopeVar2 = currentCFG->current_bb->scope;
			}
			else
			{
				scopeVar2 = currentCFG->already_defined_in_another_accessible_scope(currentCFG->current_bb->scope, var2);
			}

			if (currentCFG->already_defined_in_scope_symbol_table(currentCFG->current_bb->scope, var3))
			{
				scopeVar3 = currentCFG->current_bb->scope;
			}
			else
			{
				scopeVar3 = currentCFG->already_defined_in_another_accessible_scope(currentCFG->current_bb->scope, var3);
			}
		}
	}
	else
	{
		if (currentCFG->get_var_index(currentCFG->current_bb->scope, var2) != -1)
		{
			currentCFG->add_error("La variable " + var2 + " n'est pas déclarée. (ligne " + to_string(ctx->getStart()->getLine()) + ")\n");
		}
		else
		{
			currentCFG->add_error("La variable " + var3 + " n'est pas déclarée. (ligne " + to_string(ctx->getStart()->getLine()) + ")\n");
		}
	}

	string var1 = currentCFG->create_new_tempvar(currentCFG->current_bb->scope, "tmp");

	string total = ctx->getText();
	char operateur = total[ctx->expr()[0]->getText().length()];

	if (operateur == '>')
	{
		currentCFG->current_bb->add_IRInstr(IRInstr::Operation::cmp_lt, "int", {var1, var3, var2, to_string(scopeVar2), to_string(scopeVar3)}, currentCFG->currentScope);
	}
	else if (operateur == '<')
	{
		currentCFG->current_bb->add_IRInstr(IRInstr::Operation::cmp_lt, "int", {var1, var2, var3, to_string(scopeVar2), to_string(scopeVar3)}, currentCFG->currentScope);
	}
	return var1;
}

antlrcpp::Any buildIR::visitUnaireNegNot(ifccParser::UnaireNegNotContext *ctx)
{
	string var2 = (string)visit(ctx->expr());

	int scopeVar2;

	// On vérifie que les variables var2 et var3 sont déclarées et initialisées, sinon on lève une erreur
	if (currentCFG->get_var_index(currentCFG->current_bb->scope, var2))
	{
		if (!(currentCFG->get_var_is_initialized(currentCFG->current_bb->scope, var2)))
		{
			currentCFG->add_error("La variable " + var2 + " n'est pas initialisée. (ligne " + to_string(ctx->getStart()->getLine()) + ")\n");
		}
		else
		{
			// On récupère la portée de chacune des opérandes
			if (currentCFG->already_defined_in_scope_symbol_table(currentCFG->current_bb->scope, var2))
			{
				scopeVar2 = currentCFG->current_bb->scope;
			}
			else
			{
				scopeVar2 = currentCFG->already_defined_in_another_accessible_scope(currentCFG->current_bb->scope, var2);
			}
		}
	}
	else
	{
		currentCFG->add_error("La variable " + var2 + " n'est pas déclarée. (ligne " + to_string(ctx->getStart()->getLine()) + ")\n");
	}

	string var1 = currentCFG->create_new_tempvar(currentCFG->current_bb->scope, "tmp");

	string total = ctx->getText();
	char operateur = total[0];

	if (operateur == '-')
	{
		currentCFG->current_bb->add_IRInstr(IRInstr::Operation::copy_neg, "int", {var1, var2, to_string(scopeVar2)}, currentCFG->currentScope);
	}
	else
	{
		currentCFG->current_bb->add_IRInstr(IRInstr::Operation::copy_not, "int", {var1, var2, to_string(scopeVar2)}, currentCFG->currentScope);
	}
	return var1;
}

antlrcpp::Any buildIR::visitExprFunctionCall(ifccParser::ExprFunctionCallContext *ctx)
{
	return (string)visit(ctx->functionCall());
}

antlrcpp::Any buildIR::visitFunctionCall(ifccParser::FunctionCallContext *ctx)
{
	/*	Avant d'appeler la fonction, il faut vérifier qu'elle existe.

		Comme on ne gère pas pour l'instant les appels de fonctions des librairies, on ne vérifera pas
		que les fonctions appelées sont bien définies dans la classe avant l'appel, notamment pour garder
		l'illustration de l'appel à la méthode putchar fonctionnel

		Nous ne gérons pas non plus les variables globales. Ainsi, une déclaration anticipée de variables/fonction
		ne marchera pas.

		Cependant, si la fonction appelée est connue (i.e. dans la table des fonctions), on vérifie bien que l'appel est réalisé avec le bon
		nombre de paramètres

		Si le nombre de paramètre est supérieur à 6, on lève un message d'excuse car on ne gère pas le passage
		de paramètre sur la pile
	*/
	string var1 = currentCFG->create_new_tempvar(currentCFG->current_bb->scope, "tmp");
	string label = ctx->VARNAME()->getText();
	if (functionTable.find(label) != functionTable.end())
	{
		if (functionTable.at(label) > 0)
		{
			if (ctx->expr().size() < functionTable.at(label))
			{
				currentCFG->add_error("La fonction " + label + " a été appelée avec pas assez d'argument. (" + to_string(functionTable.at(label)) + " arguments attendus mais seulement " + to_string(ctx->expr().size()) + " ont été passés) (ligne " + to_string(ctx->getStart()->getLine()) + ")\n");
			}
			else if (ctx->expr().size() > functionTable.at(label))
			{
				currentCFG->add_error("La fonction " + label + " a été appelée avec trop d'argument. (" + to_string(functionTable.at(label)) + " arguments attendus mais " + to_string(ctx->expr().size()) + " ont été passés) (ligne " + to_string(ctx->getStart()->getLine()) + ")\n");
			}
			else if (functionTable.at(label) > 6)
			{
				currentCFG->add_error("Notre compilateur ne gère pas pour le moment les appels de fonctions avec plus de 6 arguments.");
			}
		}
	}
	vector<string> params = vector<string>();
	params.push_back(var1);
	params.push_back(label);
	for (int i = 0; i < ctx->expr().size(); i++)
	{
		string val = (string)visit(ctx->expr()[i]);
		params.push_back(val);
	}
	currentCFG->current_bb->add_IRInstr(IRInstr::Operation::call, "int", params, currentCFG->currentScope);
	return var1;
}

antlrcpp::Any buildIR::visitBlock(ifccParser::BlockContext *ctx)
{
	int initialScope = currentCFG->current_bb->scope;
	countBlock++;
	int newScope = countBlock;
	// On crée un nouveau niveau de portée
	currentCFG->currentScope = newScope;
	currentCFG->current_bb->scope = newScope;
	// On crée la table des symboles associées à la nouvelle portée
	currentCFG->create_symbol_table_scope(newScope);
	currentCFG->add_scope_relationship(newScope, initialScope);

	// On visite les statements du bloc
	visitChildren(ctx);

	// On revient à la portée initiale
	currentCFG->currentScope = initialScope;
	currentCFG->current_bb->scope = initialScope;
	return 0;
}

antlrcpp::Any buildIR::visitBlockif(ifccParser::BlockifContext *ctx)
{
	// On génère dans le basic block actuel l'assembleur correspondant à l'expression incluse dans la condition du if
	string comp = (string)visit(ctx->expr());

	currentCFG->current_bb->add_IRInstr(IRInstr::Operation::if_comp, "int", {comp, to_string(currentCFG->currentScope)}, currentCFG->currentScope);

	// On crée la structure de Basic Block qui correspond au if
	string thenLabel = "then" + to_string(countBlock);
	string elseLabel = "else" + to_string(countBlock);
	string endifLabel = "endif" + to_string(countBlock);
	BasicBlock *then = new BasicBlock(currentCFG, thenLabel, currentCFG->currentScope);
	BasicBlock *elsebb = nullptr;
	BasicBlock *endif = new BasicBlock(currentCFG, endifLabel, currentCFG->currentScope);

	// On chaîne les basic blocks entre eux
	endif->exit_true = currentCFG->current_bb->exit_true;
	endif->exit_false = currentCFG->current_bb->exit_false;
	currentCFG->current_bb->exit_true = then;
	currentCFG->current_bb->exit_false = elsebb;
	then->exit_true = endif;
	then->exit_false = nullptr;
	currentCFG->add_bb(then);
	currentCFG->add_bb(endif);
	if (ctx->blockelse())
	{
		elsebb = new BasicBlock(currentCFG, elseLabel, currentCFG->currentScope);
		elsebb->exit_true = endif;
		elsebb->exit_false = nullptr;
		currentCFG->current_bb->exit_false = elsebb;
		currentCFG->current_bb = elsebb;
		visit(ctx->blockelse());
		currentCFG->add_bb(elsebb);
	}
	currentCFG->current_bb = then;
	// On visite le block correspondant au then
	visit(ctx->block());

	currentCFG->current_bb = endif;
	return 0;
}

antlrcpp::Any buildIR::visitBlockwhile(ifccParser::BlockwhileContext *ctx)
{

	// On crée la structure de Basic Block qui correspond au while
	string whileLabel = "while" + to_string(countBlock);
	string endwhileLabel = "endwhile" + to_string(countBlock);
	BasicBlock *whilebb = new BasicBlock(currentCFG, whileLabel, currentCFG->currentScope);
	BasicBlock *endwhile = new BasicBlock(currentCFG, endwhileLabel, currentCFG->currentScope);

	endwhile->exit_true = currentCFG->current_bb->exit_true;
	endwhile->exit_false = currentCFG->current_bb->exit_false;
	currentCFG->current_bb->exit_true = whilebb;
	currentCFG->current_bb->exit_false = nullptr;
	whilebb->exit_true = whilebb;
	whilebb->exit_false = nullptr;

	currentCFG->add_bb(whilebb);
	currentCFG->add_bb(endwhile);

	currentCFG->current_bb = whilebb;
	// On génère dans le basic block du corps de la boucle l'assembleur correspondant à l'expression incluse dans la condition du while
	string comp = (string)visit(ctx->expr());
	currentCFG->current_bb->add_IRInstr(IRInstr::Operation::if_comp, "int", {comp, to_string(currentCFG->currentScope)}, currentCFG->currentScope);

	// Si la condition est vraie, on saute à la fin de la boucle
	currentCFG->current_bb->add_IRInstr(IRInstr::Operation::je, "int", {endwhileLabel}, currentCFG->currentScope);

	// Sinon, on réalise les instructions
	visit(ctx->block());

	currentCFG->current_bb = endwhile;
	return 0;
}
