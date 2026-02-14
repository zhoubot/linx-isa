// Support definitions for compiler compile-only tests.
//
// Some tests intentionally reference external symbols (e.g. to exercise
// PC-relative address materialization). When we link a standalone ELF to
// resolve relocations for raw `.bin` extraction, we provide those symbols here.

long pc_rel_var;
long pc_rel_symbol;
char data_section[64];

