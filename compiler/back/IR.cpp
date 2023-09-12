#include "IR.h"

CFG::CFG()
{
    symbolTable = unordered_map<int, unordered_map<string, infosSymbole *>*>();
    variablesInMemory = 0;
    nbTmp = 0;
}

void CFG::add_bb(BasicBlock *bb)
{
    bbs.push_back(bb);
}

void CFG::gen_asmX86(ostream &o)
{
    if(this->errors.size() != 0){
        for(auto& error: errors){
            cerr << error <<endl;
        }
        exit(1);
    } 
    // On pourrait rajouter ici la nuance entre error et warning (liste de warning incluse dans le CFG)
    else {
        for (int i = 0; i < bbs.size(); i++)
        {
            if (bbs[i]->label != "prologue" && bbs[i]->label !="epilogue")
            {
                o << "\n.globl " << bbs[i]->label << endl
                  << bbs[i]->label << ":" << endl;
                if (i == 1)
                {
                    gen_asmX86_prologue(o);
                }
                bbs[i]->gen_asmX86(o);
            }
        }
    }
}

string CFG::IR_reg_to_asm(string reg)
{
    return string();
}

void CFG::gen_asmX86_prologue(ostream &o)
{
    o <<"	pushq %rbp\n"
		"	movq %rsp, %rbp\n";
}

void CFG::gen_asmX86_epilogue(ostream &o)
{
    o <<         "	popq %rbp\n"
				 " 	ret\n";
}

void CFG::add_to_symbol_table(int scopeLevel, string name, string type, bool initialized, bool isTmp)
{
    this->variablesInMemory++;
    this->symbolTable[scopeLevel]->insert(pair<string, infosSymbole*>(name, new infosSymbole(type, initialized, isTmp, variablesInMemory * 4)));
}

bool CFG::already_defined_in_scope_symbol_table(int scopeLevel, string id){
    bool alreadyDefined = false;
    if (this->symbolTable[scopeLevel]->find(id) != this->symbolTable[scopeLevel]->end())
    {
         alreadyDefined = true;
    }
    return alreadyDefined;
}

int CFG::already_defined_in_another_accessible_scope(int scopeLevel, string id){
    int alreadyDefined = 0;
    while(scopeLevel>1) {
        scopeLevel = scopeLevelRelationship[scopeLevel];
        if(already_defined_in_scope_symbol_table(scopeLevel, id)){
            alreadyDefined = scopeLevel;
        }
    }
    return alreadyDefined;
};

string CFG::create_new_tempvar(int scopeLevel, string t)
{
    this->nbTmp++;
    string varname = "!" + t + to_string(nbTmp);
    this->add_to_symbol_table(scopeLevel, varname,"int", true, true);
    return varname;
}

void CFG::create_symbol_table_scope(int scope_level) {
    if(symbolTable.find(scope_level) == symbolTable.end()){
        symbolTable.insert(pair<int, unordered_map<string, infosSymbole*>*>(scope_level, new unordered_map<string, infosSymbole*>()));
    }
}

int CFG::get_var_index(int scopeLevel, string id)
{
    int index = -1;
    // On commence par vérifier si la variable existe dans la table des symboles associée à la portée
    if (this->symbolTable[scopeLevel]->find(id) != this->symbolTable[scopeLevel]->end())
    {
        index = this->symbolTable[scopeLevel]->find(id)->second->getOffset();
        return index;
    }
    else
    {
        // Sinon, on cherche dans les tables dont les symboles sont accessibles depuis la portée
        while (scopeLevel > 1)
        {
            scopeLevel = scopeLevelRelationship[scopeLevel];
            if (this->symbolTable[scopeLevel]->find(id) != this->symbolTable[scopeLevel]->end())
            {
                index = this->symbolTable[scopeLevel]->find(id)->second->getOffset();
                return index;
            }
        }
    }
    return index;
}

bool CFG::get_var_is_initialized(int scopeLevel, string id)
{
    bool initialized = false;
    // On commence par vérifier si la variable existe dans la table des symboles associée à la portée
    if (this->symbolTable[scopeLevel]->find(id) != this->symbolTable[scopeLevel]->end())
    {
         initialized = this->symbolTable[scopeLevel]->find(id)->second->isInitialized();
    }
    else
    {
         // Sinon, on cherche dans les tables dont les symboles sont accessibles depuis la portée
         while (scopeLevel > 1)
         {
            scopeLevel = scopeLevelRelationship[scopeLevel];
            if (this->symbolTable[scopeLevel]->find(id) != this->symbolTable[scopeLevel]->end())
            {
                initialized = this->symbolTable[scopeLevel]->find(id)->second->isInitialized();
                return initialized;
            }
         }
    }
    return initialized;
}

void CFG::set_var_is_initialized(int scopeLevel, string id)
{
    // On commence par vérifier si la variable existe dans la table des symboles associée à la portée
    if (this->symbolTable[scopeLevel]->find(id) != this->symbolTable[scopeLevel]->end())
    {
         this->symbolTable[scopeLevel]->find(id)->second->setInitialized(true);
    }
    else
    {
         // Sinon, on cherche dans les tables dont les symboles sont accessibles depuis la portée
         while (scopeLevel > 1)
         {
            scopeLevel = scopeLevelRelationship[scopeLevel];
            if (this->symbolTable[scopeLevel]->find(id) != this->symbolTable[scopeLevel]->end())
            {
                this->symbolTable[scopeLevel]->find(id)->second->setInitialized(true);
            }
         }
    }
}

void CFG::add_scope_relationship(int scope, int levelCloestAccessibleScope){
    scopeLevelRelationship.insert(pair<int, int>(scope, levelCloestAccessibleScope));
}

string CFG::new_BB_name()
{
    return string();
}

void CFG::add_error(string error)
{
    errors.push_back(error);
}

IRInstr::IRInstr(BasicBlock *bb_, Operation op, string type, vector<string> params, int scope): bb(bb_), op(op), type(type), params(params), scope(scope)
{
    comparison = false;
    if(op == if_comp){
        comparison = true;
    }
}

BasicBlock::BasicBlock(CFG *cfg, string entry_label, int scopeLevel): cfg(cfg), label(entry_label), scope(scopeLevel)
{}

void BasicBlock::gen_asmX86(ostream &o)
{
    bool comparison = false;
    if (instrs.size())
    {
        for (int i = 0; i < instrs.size(); i++)
        {
            instrs[i]->gen_asmX86(o);
        }
        comparison = instrs.back()->comparison;
    }

    if (exit_true->label == "epilogue")
    {
        cfg->gen_asmX86_epilogue(o);
    }
    else if (exit_true->label != "epilogue")
    {
        if (exit_true != nullptr && exit_false != nullptr && comparison)
        {
            o << "\tje " << exit_false->label << endl;
            o << "\tjmp " << exit_true->label << endl;
        }
        else
        {
            o << "\tjmp " << exit_true->label << endl;
        }
    }
}

void BasicBlock::add_IRInstr(IRInstr::Operation op, string type, vector<string> params, int scopeLevel)
{
    BasicBlock* bb = this;
    IRInstr * instr = new IRInstr(bb, op, type, params, scopeLevel);
    instrs.push_back(instr);
}

void IRInstr::gen_asmX86(ostream &o)
{
    this->bb->scope = scope;
    switch (this->op)
    {
        case ret:{
            int data1 = this->bb->cfg->get_var_index(this->bb->scope, params[0]);
            o << "	movl -" << data1 << "(%rbp), %eax\n";
            break;
        }
        case ldconst:{
            int data1 = this->bb->cfg->get_var_index(this->bb->scope, params[0]);
            int constante = stoi(params[1]);
            o << "	movl $" << constante << ", -"<<data1 <<"(%rbp)\n";
            break;
        }
        case copy:{
            int scope = stoi(params[2]);
            int data1 = this->bb->cfg->get_var_index(scope, params[0]);
            int data2 = this->bb->cfg->get_var_index(this->bb->scope, params[1]);
            o << "	movl -" << data2 << "(%rbp), %eax" << endl;
            o << "	movl %eax, -"<<data1 <<"(%rbp)\n";
            break;
        }
        case add:{
            int data1 = this->bb->cfg->get_var_index(this->bb->scope, params[0]);
            int data2 = this->bb->cfg->get_var_index(stoi(params[3]), params[1]);
            int data3 = this->bb->cfg->get_var_index(stoi(params[4]), params[2]);
            o << "	movl -" << data2 << "(%rbp), %eax\n";
		    o << "	addl -" << data3 << "(%rbp), %eax\n";
		    o  << "	movl %eax, -" << data1 << "(%rbp)\n";
            break;
        }
        case sub:{
            int data1 = this->bb->cfg->get_var_index(this->bb->scope, params[0]);
            int data2 = this->bb->cfg->get_var_index(stoi(params[3]), params[1]);
            int data3 = this->bb->cfg->get_var_index(stoi(params[4]), params[2]);
            o << "	movl -" << data2 << "(%rbp), %eax\n";
		    o << "	subl -" << data3 << "(%rbp), %eax\n";
		    o  << "	movl %eax, -" << data1 << "(%rbp)\n";
            break;
        }
        case mul:{
            // this->create_new_tempvar("!tmp"+id)
            int data1 = this->bb->cfg->get_var_index(this->bb->scope, params[0]);
            int data2 = this->bb->cfg->get_var_index(stoi(params[3]), params[1]);
            int data3 = this->bb->cfg->get_var_index(stoi(params[4]), params[2]);
            o << "	movl -" << data2 << "(%rbp), %eax\n";
		    o << "	imull -" << data3 << "(%rbp), %eax\n";
		    o  << "	movl %eax, -" << data1 << "(%rbp)\n";
            break;
        }
        case div:{
            // this->create_new_tempvar("!tmp"+id)
            int data1 = this->bb->cfg->get_var_index(this->bb->scope, params[0]);
            int data2 = this->bb->cfg->get_var_index(stoi(params[3]), params[1]);
            int data3 = this->bb->cfg->get_var_index(stoi(params[4]), params[2]);

            o << "	movl -" << data2 << "(%rbp), %eax"<<endl;
            o << "  cltd"<<endl;
            o << "  idivl -" << data3 << "(%rbp)"<<endl;
            o << "	movl %eax, -" << data1 << "(%rbp)"<<endl;
            break;
        }      
        case call:{

            int data1 = this->bb->cfg->get_var_index(this->bb->scope, params[0]);
            string label_function = this->params[1];
            int nb_params_function = this->params.size()-2;
            int stackPointerOffset = ((this->params.size()-2))*6*16;

            o << "	subq $" << stackPointerOffset <<" , %rsp" << endl;
            if(nb_params_function>=6){
                int param6 = this->bb->cfg->get_var_index(this->bb->scope, params[7]);
                o << "	movl -" << param6 <<"(%rbp), %r9d" << endl;
            }
            if(nb_params_function>=5){
                int param5 = this->bb->cfg->get_var_index(this->bb->scope, params[6]);
                o << "	movl -" << param5 << "(%rbp), %r8d" << endl;
            }
            if(nb_params_function>=4){
                int param4 = this->bb->cfg->get_var_index(this->bb->scope, params[5]);
                o << "	movl -" << param4 <<"(%rbp), %ecx" << endl;
            }
            if(nb_params_function>=3){
                int param3 = this->bb->cfg->get_var_index(this->bb->scope, params[4]);
                o << "	movl -" << param3 <<"(%rbp), %edx" << endl;
            }
            if(nb_params_function>=2){
                int param2 = this->bb->cfg->get_var_index(this->bb->scope, params[3]);
                o << "	movl -" <<  param2 << "(%rbp), %esi" << endl;
            }
            if(nb_params_function>=1){
                int param1 = this->bb->cfg->get_var_index(this->bb->scope, params[2]);
                o << "	movl -" <<  param1 << "(%rbp), %edi" << endl;
            }

            o << "	call " << label_function << endl;
	        o << "	addq $" << stackPointerOffset <<" , %rsp" << endl;
            o << "	movl %eax, -" <<  data1 << "(%rbp)" << endl;
            break;
        }
        case cmp_eq:{
            int data3 = this->bb->cfg->get_var_index(this->bb->scope, params[0]);
            int data1 = this->bb->cfg->get_var_index(stoi(params[3]), params[1]);
            int data2 = this->bb->cfg->get_var_index(stoi(params[4]), params[2]);
            o << "	movl -" << data1 << "(%rbp), %eax\n";
            o << "	cmpl -" << data2 << "(%rbp), %eax\n";
            o << "	sete %al\n";
            o << "	movzbl	%al, %eax\n";
            o << "	movl	%eax, -" << data3 << "(%rbp)\n";
            break;
        }
        case cmp_lt:{
            int data3 = this->bb->cfg->get_var_index(this->bb->scope, params[0]);
            int data1 = this->bb->cfg->get_var_index(stoi(params[3]), params[1]);
            int data2 = this->bb->cfg->get_var_index(stoi(params[4]), params[2]);

            // On mets dans %al la valeur telle que a >= b
            o << "	movl -" << data1 << "(%rbp), %eax\n";
            o << "	cmpl %eax, -" << data2 << "(%rbp)\n";
            o << "	setle %al\n";

            // On inverse le résultat
            o << "	movzbl	%al, %eax\n";
            o << "	cmpl $0, %eax\n";
            o << "	sete %al\n";

            o << "	movzbl	%al, %eax\n";
            o << "	movl	%eax, -" << data3 << "(%rbp)\n";
            break;
        }
        case cmp_le:{
            int data3 = this->bb->cfg->get_var_index(this->bb->scope, params[0]);
            int data1 = this->bb->cfg->get_var_index(stoi(params[3]), params[1]);
            int data2 = this->bb->cfg->get_var_index(stoi(params[4]), params[2]);

            // On mets dans %al la valeur telle que a <= b
            o << "	movl -" << data1 << "(%rbp), %eax\n";
            o << "	cmpl -" << data2 << "(%rbp), %eax\n";
            o << "	setle %al\n";

            o << "	movzbl	%al, %eax\n";
            o << "	movl	%eax, -" << data3 << "(%rbp)\n";
            break;
        }
        case copy_not:{
            // On  récupère la case mémoire contenant le résultat
            int data1 = this->bb->cfg->get_var_index(this->bb->scope, params[0]);
            int data2 = this->bb->cfg->get_var_index(stoi(params[2]), params[1]);
            o << "	movl -" << data2 << "(%rbp), %eax\n";
            o << "	cmpl $0, %eax\n";
            o << "	sete %al\n";
            o << "	movzbl	%al, %eax\n";
            o << "	movl	%eax, -" << data1 << "(%rbp)\n";
            break;
        }
        case copy_neg:{
            int data1 = this->bb->cfg->get_var_index(this->bb->scope, params[0]);
            int data2 = this->bb->cfg->get_var_index(stoi(params[2]), params[1]);
            o << "	movl -" << data2 << "(%rbp), %eax\n";
            o << "	neg %eax\n";
            o << "	movl %eax, -" << data1 << "(%rbp)\n";
            break;
        }
        case function_params_initialisation:{
            int nb_params_function = this->params.size();
            if(nb_params_function>=6){
                int param6 = this->bb->cfg->get_var_index(this->bb->scope, params[5]);
                o << "	movl %r9d, -" << param6 <<"(%rbp)" << endl;
            }
            if(nb_params_function>=5){
                int param5 = this->bb->cfg->get_var_index(this->bb->scope, params[4]);
                o << "	movl %r8d, -" << param5 <<"(%rbp)" << endl;
            }
            if(nb_params_function>=4){
                int param4 = this->bb->cfg->get_var_index(this->bb->scope, params[3]);
                o << "	movl %ecx, -" << param4 <<"(%rbp)" << endl;
            }
            if(nb_params_function>=3){
                int param3 = this->bb->cfg->get_var_index(this->bb->scope, params[2]);
                o << "	movl %edx, -" << param3 <<"(%rbp)" << endl;
            }
            if(nb_params_function>=2){
                int param2 = this->bb->cfg->get_var_index(this->bb->scope, params[1]);
                o << "	movl %esi, -" <<  param2 << "(%rbp)" << endl;
            }
            if(nb_params_function>=1){
                int param1 = this->bb->cfg->get_var_index(this->bb->scope, params[0]);
                o << "	movl %edi, -" <<  param1 << "(%rbp)" << endl;
            }
            break;
        }
        case if_comp:{
            int param1 = this->bb->cfg->get_var_index(stoi(params[1]), params[0]);
            o << "\tcmpl $0, -" << param1 <<"(%rbp)"<< endl;
            break;
        }
        case jne:{
                string label = params[0];
                o << "\tjne " << label << endl;
            break;
        }
        case je:{
            string label = params[0];
                o << "\tje " << label << endl;
            break;
        }
        case jmp:{
            string label = params[0];
            o << "\tjmp "<< label << endl;
        }
        default:{break;}
    }
}
