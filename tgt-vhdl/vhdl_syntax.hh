/*
 *  VHDL abstract syntax elements.
 *
 *  Copyright (C) 2008  Nick Gasson (nick@nickg.me.uk)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef INC_VHDL_SYNTAX_HH
#define INC_VHDL_SYNTAX_HH

#include "vhdl_element.hh"
#include "vhdl_type.hh"

class vhdl_scope;
class vhdl_entity;
class vhdl_arch;

class vhdl_expr : public vhdl_element {
public:
   vhdl_expr(vhdl_type* type, bool isconst=false)
      : type_(type), isconst_(isconst) {}
   virtual ~vhdl_expr();

   const vhdl_type *get_type() const { return type_; }
   bool constant() const { return isconst_; }
   virtual vhdl_expr *cast(const vhdl_type *to);
   virtual vhdl_expr *resize(int newwidth);
private:
   vhdl_type *type_;
   bool isconst_;
};


/*
 * A scalar or array variable reference.
 */
class vhdl_var_ref : public vhdl_expr {
public:
   vhdl_var_ref(const char *name, vhdl_type *type,
                vhdl_expr *slice = NULL)
      : vhdl_expr(type), name_(name), slice_(slice) {}
   ~vhdl_var_ref();
   
   void emit(std::ostream &of, int level) const;
   const std::string &get_name() const { return name_; }
   void set_slice(vhdl_expr *s) { slice_ = s; }
private:
   std::string name_;
   vhdl_expr *slice_;
};


enum vhdl_binop_t {
   VHDL_BINOP_AND = 0,
   VHDL_BINOP_OR,
   VHDL_BINOP_EQ,
   VHDL_BINOP_NEQ,
   VHDL_BINOP_ADD,
   VHDL_BINOP_SUB,
   VHDL_BINOP_MULT,
   VHDL_BINOP_LT,
   VHDL_BINOP_GT,
   VHDL_BINOP_SL,
   VHDL_BINOP_SR,
   VHDL_BINOP_XOR,
};

/*
 * A binary expression contains a list of operands rather
 * than just two: this is to model n-input gates and the
 * like. A second constructor is provided to handle the
 * common case of a true binary expression.
 */
class vhdl_binop_expr : public vhdl_expr {
public:
   vhdl_binop_expr(vhdl_binop_t op, vhdl_type *type)
      : vhdl_expr(type), op_(op) {}
   vhdl_binop_expr(vhdl_expr *left, vhdl_binop_t op,
                   vhdl_expr *right, vhdl_type *type);
   ~vhdl_binop_expr();

   void add_expr(vhdl_expr *e);
   void emit(std::ostream &of, int level) const;
private:
   std::list<vhdl_expr*> operands_;
   vhdl_binop_t op_;
};


enum vhdl_unaryop_t {
   VHDL_UNARYOP_NOT,
};

class vhdl_unaryop_expr : public vhdl_expr {
public:
   vhdl_unaryop_expr(vhdl_unaryop_t op, vhdl_expr *operand,
                     vhdl_type *type)
      : vhdl_expr(type), op_(op), operand_(operand) {}
   ~vhdl_unaryop_expr();

   void emit(std::ostream &of, int level) const;
private:
   vhdl_unaryop_t op_;
   vhdl_expr *operand_;
};


class vhdl_const_string : public vhdl_expr {
public:
   vhdl_const_string(const char *value)
      : vhdl_expr(vhdl_type::string(), true), value_(value) {}

   void emit(std::ostream &of, int level) const;
private:
   std::string value_;
};

class vhdl_const_bits : public vhdl_expr {
public:
   vhdl_const_bits(const char *value, int width, bool issigned);
   void emit(std::ostream &of, int level) const;
   const std::string &get_value() const { return value_; }
   vhdl_expr *cast(const vhdl_type *to);
private:
   std::string value_;
   bool qualified_, signed_;
};

class vhdl_const_bit : public vhdl_expr {
public:
   vhdl_const_bit(char bit)
      : vhdl_expr(vhdl_type::std_logic(), true), bit_(bit) {}
   void emit(std::ostream &of, int level) const;
private:
   char bit_;
};

enum time_unit_t {
   TIME_UNIT_NS,
};

class vhdl_const_time : public vhdl_expr {
public:
   vhdl_const_time(int64_t value, time_unit_t units)
      : vhdl_expr(vhdl_type::time(), true), value_(value), units_(units) {}
   void emit(std::ostream &of, int level) const;
private:
   int64_t value_;
   time_unit_t units_;
};

class vhdl_const_int : public vhdl_expr {
public:
   vhdl_const_int(int64_t value)
      : vhdl_expr(vhdl_type::integer(), true), value_(value) {}
   void emit(std::ostream &of, int level) const;
private:
   int64_t value_;
};

class vhdl_expr_list : public vhdl_element {
public:
   ~vhdl_expr_list();
   
   void emit(std::ostream &of, int level) const;
   bool empty() const { return exprs_.empty(); }
   void add_expr(vhdl_expr *e);
private:
   std::list<vhdl_expr*> exprs_;
};


/*
 * A function call within an expression.
 */
class vhdl_fcall : public vhdl_expr {
public:
   vhdl_fcall(const char *name, vhdl_type *rtype)
      : vhdl_expr(rtype), name_(name) {};
   ~vhdl_fcall() {}

   void add_expr(vhdl_expr *e) { exprs_.add_expr(e); }
   void emit(std::ostream &of, int level) const;
private:
   std::string name_;
   vhdl_expr_list exprs_;
};

/*
 * A concurrent statement appears in architecture bodies but not
 * processes.
 */
class vhdl_conc_stmt : public vhdl_element {
public:
   virtual ~vhdl_conc_stmt() {}
};

typedef std::list<vhdl_conc_stmt*> conc_stmt_list_t;

/*
 * A concurrent signal assignment (i.e. not part of a process).
 */
class vhdl_cassign_stmt : public vhdl_conc_stmt {
public:
   vhdl_cassign_stmt(vhdl_var_ref *lhs, vhdl_expr *rhs)
      : lhs_(lhs), rhs_(rhs) {}
   ~vhdl_cassign_stmt();

   void emit(std::ostream &of, int level) const;
private:
   vhdl_var_ref *lhs_;
   vhdl_expr *rhs_;
};

/*
 * Any sequential statement in a process.
 */
class vhdl_seq_stmt : public vhdl_element {
public:
   virtual ~vhdl_seq_stmt() {}
};


/*
 * A list of sequential statements. For example inside a
 * process, loop, or if statement.
 */
class stmt_container {
public:
   ~stmt_container();
   
   void add_stmt(vhdl_seq_stmt *stmt);
   void emit(std::ostream &of, int level) const;
   bool empty() const { return stmts_.empty(); }
private:
   std::list<vhdl_seq_stmt*> stmts_;
};


/*
 * Shared between blocking and non-blocking assignment.
 */
class vhdl_abstract_assign_stmt : public vhdl_seq_stmt {
public:
   vhdl_abstract_assign_stmt(vhdl_var_ref *lhs, vhdl_expr *rhs)
      : lhs_(lhs), rhs_(rhs), after_(NULL) {}
   virtual ~vhdl_abstract_assign_stmt();

   void set_after(vhdl_expr *after) { after_ = after; }
protected:
   vhdl_var_ref *lhs_;
   vhdl_expr *rhs_, *after_;
};                                                      


/*
 * Similar to Verilog non-blocking assignment, except the LHS
 * must be a signal not a variable.
 */
class vhdl_nbassign_stmt : public vhdl_abstract_assign_stmt {
public:
   vhdl_nbassign_stmt(vhdl_var_ref *lhs, vhdl_expr *rhs)
      : vhdl_abstract_assign_stmt(lhs, rhs) {}
   
   void emit(std::ostream &of, int level) const;
};


class vhdl_assign_stmt : public vhdl_abstract_assign_stmt {
public:
   vhdl_assign_stmt(vhdl_var_ref *lhs, vhdl_expr *rhs)
      : vhdl_abstract_assign_stmt(lhs, rhs) {}

   void emit(std::ostream &of, int level) const;
};


enum vhdl_wait_type_t {
   VHDL_WAIT_INDEF,  // Suspend indefinitely
   VHDL_WAIT_FOR,    // Wait for a constant amount of time
};

/*
 * Delay simulation indefinitely, until an event, or for a
 * specified time.
 */
class vhdl_wait_stmt : public vhdl_seq_stmt {
public:
   vhdl_wait_stmt(vhdl_wait_type_t type = VHDL_WAIT_INDEF,
                  vhdl_expr *expr = NULL)
      : type_(type), expr_(expr) {}
   ~vhdl_wait_stmt();
   
   void emit(std::ostream &of, int level) const;
private:
   vhdl_wait_type_t type_;
   vhdl_expr *expr_;
};


class vhdl_null_stmt : public vhdl_seq_stmt {
public:
   void emit(std::ostream &of, int level) const;
};


class vhdl_assert_stmt : public vhdl_seq_stmt {
public:
   vhdl_assert_stmt(const char *reason)
      : reason_(reason) {}

   void emit(std::ostream &of, int level) const;
private:
   std::string reason_;
};


class vhdl_if_stmt : public vhdl_seq_stmt {
public:
   vhdl_if_stmt(vhdl_expr *test);
   ~vhdl_if_stmt();

   stmt_container *get_then_container() { return &then_part_; }
   stmt_container *get_else_container() { return &else_part_; }
   void emit(std::ostream &of, int level) const;
private:
   vhdl_expr *test_;
   stmt_container then_part_, else_part_;
};


/*
 * A single branch in a case statement consisting of an
 * expression part and a statement container.
 */
class vhdl_case_branch : public vhdl_element {
public:
   vhdl_case_branch(vhdl_expr *when) : when_(when) {}
   ~vhdl_case_branch();

   stmt_container *get_container() { return &stmts_; }
   void emit(std::ostream &of, int level) const;
private:
   vhdl_expr *when_;
   stmt_container stmts_;
};

typedef std::list<vhdl_case_branch*> case_branch_list_t;

class vhdl_case_stmt : public vhdl_seq_stmt {
public:
   vhdl_case_stmt(vhdl_expr *test) : test_(test) {}
   ~vhdl_case_stmt();

   void add_branch(vhdl_case_branch *b) { branches_.push_back(b); }
   void emit(std::ostream &of, int level) const;
private:
   vhdl_expr *test_;
   case_branch_list_t branches_;
};


class vhdl_while_stmt : public vhdl_seq_stmt {
public:
   vhdl_while_stmt(vhdl_expr *test) : test_(test) {}
   ~vhdl_while_stmt();

   stmt_container *get_container() { return &stmts_; }
   void emit(std::ostream &of, int level) const;
private:
   vhdl_expr *test_;
   stmt_container stmts_;
};


/*
 * A procedure call. Which is a statement, unlike a function
 * call which is an expression.
 */
class vhdl_pcall_stmt : public vhdl_seq_stmt {
public:
   vhdl_pcall_stmt(const char *name) : name_(name) {}
   
   void emit(std::ostream &of, int level) const;
   void add_expr(vhdl_expr *e) { exprs_.add_expr(e); }
private:
   std::string name_;
   vhdl_expr_list exprs_;
};


/*
 * A declaration of some sort (variable, component, etc.).
 * Declarations have names, which is the identifier of the variable,
 * constant, etc. not the type.
 */
class vhdl_decl : public vhdl_element {
public:
   vhdl_decl(const char *name, vhdl_type *type = NULL,
             vhdl_expr *initial = NULL)
      : name_(name), type_(type), initial_(initial) {}
   virtual ~vhdl_decl();

   const std::string &get_name() const { return name_; }
   const vhdl_type *get_type() const;
   void set_type(vhdl_type *t) { type_ = t; }
   void set_initial(vhdl_expr *initial);
   bool has_initial() const { return initial_ != NULL; }
protected:
   std::string name_;
   vhdl_type *type_;
   vhdl_expr *initial_;
};

typedef std::list<vhdl_decl*> decl_list_t;
   

/*
 * A forward declaration of a component. At the moment it is assumed
 * that components declarations will only ever be for entities
 * generated by this code generator. This is enforced by making the
 * constructor private (use component_decl_for instead).
 */
class vhdl_component_decl : public vhdl_decl {
public:
   static vhdl_component_decl *component_decl_for(vhdl_entity *ent);

   void emit(std::ostream &of, int level) const;
private:
   vhdl_component_decl(const char *name);

   decl_list_t ports_;
};


/*
 * A variable declaration inside a process (although this isn't
 * enforced here).
 */
class vhdl_var_decl : public vhdl_decl {
public:
   vhdl_var_decl(const char *name, vhdl_type *type)
      : vhdl_decl(name, type) {}
   void emit(std::ostream &of, int level) const;
};


/*
 * A signal declaration in architecture.
 */
class vhdl_signal_decl : public vhdl_decl {
public:
   vhdl_signal_decl(const char *name, vhdl_type *type)
      : vhdl_decl(name, type) {}
   virtual void emit(std::ostream &of, int level) const;
};


/*
 * A parameter to a function.
 */
class vhdl_param_decl : public vhdl_decl {
public:
   vhdl_param_decl(const char *name, vhdl_type *type)
      : vhdl_decl(name, type) {}
   void emit(std::ostream &of, int level) const;
};

enum vhdl_port_mode_t {
   VHDL_PORT_IN,
   VHDL_PORT_OUT,
   VHDL_PORT_INOUT,
};

/*
 * A port declaration is like a signal declaration except
 * it has a direction and appears in the entity rather than
 * the architecture.
 */
class vhdl_port_decl : public vhdl_decl {
public:
   vhdl_port_decl(const char *name, vhdl_type *type,
                  vhdl_port_mode_t mode)
      : vhdl_decl(name, type), mode_(mode) {}

   void emit(std::ostream &of, int level) const;
private:
   vhdl_port_mode_t mode_;
};

/*
 * A mapping from port name to an expression.
 */
struct port_map_t {
   std::string name;
   vhdl_expr *expr;
};

typedef std::list<port_map_t> port_map_list_t;

/*
 * Instantiation of component. This is really only a placeholder
 * at the moment until the port mappings are worked out.
 */
class vhdl_comp_inst : public vhdl_conc_stmt {
public:
   vhdl_comp_inst(const char *inst_name, const char *comp_name);
   ~vhdl_comp_inst();

   void emit(std::ostream &of, int level) const;
   void map_port(const char *name, vhdl_expr *expr);
private:
   std::string comp_name_, inst_name_;
   port_map_list_t mapping_;
};


/*
 * Contains a list of declarations in a hierarchy.
 * A scope can be `initializing' where assignments automatically
 * create initial values for declarations.
 */
class vhdl_scope {
public:
   vhdl_scope();
   ~vhdl_scope();
   
   void add_decl(vhdl_decl *decl);
   vhdl_decl *get_decl(const std::string &name) const;
   bool have_declared(const std::string &name) const;
   vhdl_scope *get_parent();
   
   bool empty() const { return decls_.empty(); }
   const decl_list_t &get_decls() const { return decls_; }
   void set_parent(vhdl_scope *p) { parent_ = p; }

   bool initializing() const { return init_; }
   void set_initializing(bool i) { init_ = i; }

   void set_allow_signal_assignment(bool b) { sig_assign_ = b; }
   bool allow_signal_assignment() const { return sig_assign_; }
private:
   decl_list_t decls_;
   vhdl_scope *parent_;
   bool init_, sig_assign_;
};


/*
 * Any sort of procedural element: process, function, or
 * procedure. Roughly these map onto Verilog's processes,
 * functions, and tasks.
 */
class vhdl_procedural {
public:
   virtual ~vhdl_procedural() {}
   
   virtual stmt_container *get_container() { return &stmts_; }
   virtual vhdl_scope *get_scope() { return &scope_; }
protected:
   stmt_container stmts_;
   vhdl_scope scope_;
};


class vhdl_function : public vhdl_decl, public vhdl_procedural {
public:
   vhdl_function(const char *name, vhdl_type *ret_type);
   
   void emit(std::ostream &of, int level) const;
   vhdl_scope *get_scope() { return &variables_; }
   void add_param(vhdl_param_decl *p) { scope_.add_decl(p); }
private:
   vhdl_scope variables_;
};


class vhdl_process : public vhdl_conc_stmt, public vhdl_procedural {
public:
   vhdl_process(const char *name = "") : name_(name) {}

   void emit(std::ostream &of, int level) const;
   void add_sensitivity(const char *name);
private:
   std::string name_;
   string_list_t sens_;
};


/*
 * An architecture which implements an entity.
 */
class vhdl_arch : public vhdl_element {
public:
   vhdl_arch(const char *entity, const char *name)
      : name_(name), entity_(entity) {}
   virtual ~vhdl_arch();

   void emit(std::ostream &of, int level=0) const;
   void add_stmt(vhdl_process *proc);
   void add_stmt(vhdl_conc_stmt *stmt);
   vhdl_scope *get_scope() { return &scope_; }
private:
   conc_stmt_list_t stmts_;
   vhdl_scope scope_;
   std::string name_, entity_;
};

/*
 * An entity defines the ports, parameters, etc. of a module. Each
 * entity is associated with a single architecture (although
 * technically this need not be the case).  Entities are `derived'
 * from instantiations of Verilog module scopes in the hierarchy.
 */
class vhdl_entity : public vhdl_element {
public:
   vhdl_entity(const char *name, const char *derived_from,
               vhdl_arch *arch);
   virtual ~vhdl_entity();

   void emit(std::ostream &of, int level=0) const;
   void add_port(vhdl_port_decl *decl);
   vhdl_arch *get_arch() const { return arch_; }
   const std::string &get_name() const { return name_; }
   const std::string &get_derived_from() const { return derived_from_; }

   vhdl_scope *get_scope() { return &ports_; }
private:
   std::string name_;
   vhdl_arch *arch_;  // Entity may only have a single architecture
   std::string derived_from_;
   vhdl_scope ports_;
};

typedef std::list<vhdl_entity*> entity_list_t;

#endif
