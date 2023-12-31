#ifndef IR_H
#define IR_H

#include <vector>
#include <string>
#include <iostream>
#include <initializer_list>
#include <utility>
#include <list>
#include <unordered_map>

using namespace std;

class BasicBlock;
class CFG;

// Classe définissant les entrées dans la table des symboles
class infosSymbole
{
private:
	std::string type;
	bool initialized;
	bool isTmp;
	int offset;

public:
	infosSymbole(std::string type, bool initialized, bool isTmp, int offset) : type(type), initialized(initialized), isTmp(isTmp), offset(offset) {}
	infosSymbole() {}
	int getOffset()
	{
		return offset;
	}
	void setOffset(int of)
	{
		offset = of;
	}
	bool isInitialized()
	{
		return initialized;
	}
	void setInitialized(bool init)
	{
		initialized = init;
	}
};

//! The class for one 3-address instruction
class IRInstr
{

public:
	/** The instructions themselves -- feel free to subclass instead */
	typedef enum
	{
		ret,
		ldconst,
		copy,
		add,
		sub,
		mul,
		div,
		rmem,
		wmem,
		call,
		cmp_eq,
		cmp_lt,
		cmp_le,
		copy_not,
		copy_neg,
		function_params_initialisation,
		if_comp,
		jne,
		je,
		jmp,
	} Operation;

	/**  constructor */
	IRInstr(BasicBlock *bb_, Operation op, string type, vector<string> params, int scope);

	/** Actual code generation */
	void gen_asmX86(ostream &o); /**< x86 assembly code generation for this IR instruction */

	BasicBlock *bb; /**< The BB this instruction belongs to, which provides a pointer to the CFG this instruction belong to */
	Operation op;
	string type;
	bool comparison;
	int scope;
	vector<string> params; /**< For 3-op instrs: d, x, y; for ldconst: d, c;  For call: label, d, params;  for wmem and rmem: choose yourself */
						   // if you subclass IRInstr, each IRInstr subclass has its parameters and the previous (very important) comment becomes useless: it would be a better design.
};

/**  The class for a basic block */

/* A few important comments.
	 IRInstr has no jump instructions.
	 cmp_* instructions behaves as an arithmetic two-operand instruction (add or mult),
	  returning a boolean value (as an int)

	 Assembly jumps are generated as follows:
	 BasicBlock::gen_asm() first calls IRInstr::gen_asm() on all its instructions, and then
			if  exit_true  is a  nullptr,
			the epilogue is generated
		else if exit_false is a nullptr,
		  an unconditional jmp to the exit_true branch is generated
				else (we have two successors, hence a branch)
		  an instruction comparing the value of test_var_name to true is generated,
					followed by a conditional branch to the exit_false branch,
					followed by an unconditional branch to the exit_true branch
	 The attribute test_var_name itself is defined when converting
  the if, while, etc of the AST to IR.

Possible optimization:
	 a cmp_* comparison instructions, if it is the last instruction of its block,
	   generates an actual assembly comparison
	   followed by a conditional jump to the exit_false branch
*/

class BasicBlock
{
public:
	BasicBlock(CFG *cfg, string entry_label, int scopeLevel);
	void gen_asmX86(ostream &o); /**< x86 assembly code generation for this basic block (very simple) */

	void add_IRInstr(IRInstr::Operation op, string type, vector<string> params, int scope);

	// No encapsulation whatsoever here. Feel free to do better.
	int scope;
	BasicBlock *exit_true;	  /**< pointer to the next basic block, true branch. If nullptr, return from procedure */
	BasicBlock *exit_false;	  /**< pointer to the next basic block, false branch. If null_ptr, the basic block ends with an unconditional jump */
	string label;			  /**< label of the BB, also will be the label in the generated code */
	CFG *cfg;				  /** < the CFG where this block belongs */
	vector<IRInstr *> instrs; /** < the instructions themselves. */
	string test_var_name;	  /** < when generating IR code for an if(expr) or while(expr) etc,
														store here the name of the variable that holds the value of expr */
};

/** The class for the control flow graph, also includes the symbol table */

/* A few important comments:
	 The entry block is the one with the same label as the AST function name.
	   (it could be the first of bbs, or it could be defined by an attribute value)
	 The exit block is the one with both exit pointers equal to nullptr.
	 (again it could be identified in a more explicit way)

 */
class CFG
{
public:
	CFG();

	void add_bb(BasicBlock *bb);

	// x86 code generation: could be encapsulated in a processor class in a retargetable compiler
	void gen_asmX86(ostream &o);
	string IR_reg_to_asm(string reg); /**< helper method: inputs a IR reg or input variable, returns e.g. "-24(%rbp)" for the proper value of 24 */
	void gen_asmX86_prologue(ostream &o);
	void gen_asmX86_epilogue(ostream &o);

	// symbol table methods
	void add_to_symbol_table(int scopeLevel, string name, string type, bool initialized, bool isTmp);
    bool already_defined_in_scope_symbol_table(int scopeLevel, string id);
    int already_defined_in_another_accessible_scope(int scopeLevel, string id);
    string create_new_tempvar(int scopeLevel, string t);
    void create_symbol_table_scope(int scope_level);
    int get_var_index(int scopeLevel, string name);
    bool get_var_is_initialized(int scopeLevel, string id);
    void set_var_is_initialized(int scopeLevel, string id);
    void add_scope_relationship(int scope, int levelCloestAccessibleScope);

    // basic block management
	string new_BB_name();
	BasicBlock *current_bb;

	/** Error : variable already declared */
	void add_error(string error);

	// nom du cfg en public
	string label;
	int currentScope;

protected:
	// Table des symboles
	unordered_map<int, unordered_map<string, infosSymbole *>*> symbolTable;
	unordered_map<int, int> scopeLevelRelationship;
	int variablesInMemory; /**< to allocate new symbols in the symbol table */
	int nbTmp;
	int nextBBnumber;		  /**< just for naming */
	vector<BasicBlock *> bbs; /**< all the basic blocks of this CFG*/
	list<string> errors;
};

#endif