#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>

#include "GAS_utility.h"


size_t label_cnt, tmp_cnt, var_cnt;
FILE *debug;

char s[32768];
typedef struct IR_list
{
    char *ss[10];
    struct IR_list *next, *prev;
} IR_list;

char ss[10][32768];
int lss;

void debug_IR_list(IR_list *ir)
{
    for (int i = 0; ir->ss[i] != NULL; i++)
        fprintf(debug, "%s ", ir->ss[i]);
    fprintf(debug, "\n");
    fflush(debug);
}

IR_list *new_IR_list(IR_list *prev)
{
    IR_list *p = (IR_list *)malloc(sizeof(IR_list));
    for (int i = 0; i < 10; i++)
    {
        size_t ll = strlen(ss[i]);
        if (ll == 0 || i>=lss)
        {
            p->ss[i] = NULL;
        }
        else
        {
            p->ss[i] = (char *)malloc(sizeof(char) * (ll + 1));
            strcpy(p->ss[i], ss[i]);
        }
    }
    p->next = NULL;
    p->prev = prev;
    if (prev != NULL)
    {
        prev->next = p;
    }
    return p;
}

inline static char is_ws(const char c)
{
    return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}

void split_str(const char *s)
{
    int l1 = 0, l2 = 0, ls = 0;
    while (s[ls])
    {
        while (is_ws(s[ls]))
            ++ls;

        while (s[ls] && !is_ws(s[ls]))
        {
            ss[l1][l2] = s[ls];
            ++l2;
            ++ls;
        }
        ss[l1][l2] = 0;
        l2 = 0;
        ++l1;
    }
    lss=l1;
}

void del_list(IR_list *p)
{
    if (p->prev != NULL)
    {
        p->prev->next = p->next;
    }
    if (p->next != NULL)
    {
        p->next->prev = p->prev;
    }
    for(int i=0;i<10;++i)
        if(p->ss[i] != NULL)
            free(p->ss[i]);
    free(p);
}

size_t get_list_len(IR_list *ir)
{
    for (int i = 0; ; i++)
        if (ir->ss[i] == NULL)
            return i;
}

bool opt_if(IR_list *u)
{
    bool flg = false;
    while (u != NULL)
    {
        if (strcmp(u->ss[0], "IF") == 0)
        {
            // if (u->next != NULL && strcmp(u->next->ss[0], "GOTO") == 0 &&
            //     u->next->next != NULL && strcmp(u->next->next->ss[0], "LABEL") == 0)
            // {
            //     if (strcmp(u->ss[5], u->next->next->ss[1]) == 0)
            //     {
            //         free(u->ss[5]);
            //         int ll = strlen(u->next->ss[1]);
            //         u->ss[5] = (char *)malloc(sizeof(char) * (ll + 1));
            //         strcpy(u->ss[5], u->next->ss[1]);
            //         u->ss[5][ll] = 0;

            //         del_list(u->next);

            //         if (u->ss[2][0] == '=')
            //         {
            //             u->ss[2][0] = '!';
            //         }
            //         else if (u->ss[2][0] == '!')
            //         {
            //             u->ss[2][0] = '=';
            //         }
            //         else if (u->ss[2][0] == '<')
            //         {
            //             free(u->ss[2]);
            //             if(u->ss[2][1] == '='){
            //                 u->ss[2] = (char*)malloc(sizeof(char)*2);
            //                 u->ss[2][0]='>';
            //                 u->ss[2][1]=0;
            //             } else {
            //                 u->ss[2] = (char*)malloc(sizeof(char)*3);
            //                 u->ss[2][0]='>';
            //                 u->ss[2][1]='=';
            //                 u->ss[2][2]=0;
            //             }
            //         }
            //         else if (u->ss[2][0] == '>')
            //         {
            //             free(u->ss[2]);
            //             if(u->ss[2][1] == '='){
            //                 u->ss[2] = (char*)malloc(sizeof(char)*2);
            //                 u->ss[2][0]='<';
            //                 u->ss[2][1]=0;
            //             } else {
            //                 u->ss[2] = (char*)malloc(sizeof(char)*3);
            //                 u->ss[2][0]='<';
            //                 u->ss[2][1]='=';
            //                 u->ss[2][2]=0;
            //             }
            //         }
            //         flg = 1;
            //     }
            // }
            if (u->next != NULL && strcmp(u->next->ss[0], "LABEL") == 0){
                if(strcmp(u->ss[5], u->next->ss[1]) == 0){
                    /* IF xxx GOTO Lu
                       LABEL: Lu*/
                    u=u->next;
                    del_list(u->prev);
                    flg = true;
                }
            }
        }
        if(strcmp(u->ss[0], "GOTO") == 0){
            if (u->next != NULL && strcmp(u->next->ss[0], "LABEL") == 0){
                if(strcmp(u->ss[1], u->next->ss[1]) == 0){
                    /* GOTO Lu
                       LABEL: Lu*/
                    u=u->next;
                    del_list(u->prev);
                    flg = true;
                }
            }
        }
        u = u->next;
    }
    return flg;
}


struct _Var;
typedef struct _Parent
{
    struct _Var *var;
    IR_list *recent;
} Parent;

typedef struct _Var
{
    enum
    {
        VAR,
        CONST,
        FUNC,
    } type;

    // char name[7]; // "?65535\0"
    IR_list *recent_ir;
    int val;
    struct _Parent parent[2];
} Var;



Var v_var[USHRT_MAX], t_var[USHRT_MAX];
bool useful_v[USHRT_MAX], useful_t[USHRT_MAX];


Var *new_constant_var(const char *s)
{
    Var *var = (Var *)malloc(sizeof(Var));
    var->type = CONST;
    var->val = atoi(s);
    for (size_t i = 0; i < 2; i++)
        var->parent[i] = (Parent){NULL, NULL};
    return var;
}

Var *get_var(const char *name)
{
    switch (name[0])
    {
        case 'v':
            return &v_var[atoi(name + 1)];
        case 't':
            return &t_var[atoi(name + 1)];
        case '#':
            return new_constant_var(name + 1);
        case '&':
        case '*':
            return get_var(name + 1);
    }
}

bool equal_parent(Parent a, Parent b)
{
    if (a.var == NULL && b.var == NULL)
        return true;
    if ((a.var == NULL) + (b.var == NULL) == 1)
        return false;
    return (a.var == b.var && a.recent == b.recent) ||
        (a.var->type == CONST && b.var->type == CONST && a.var->val == b.var->val);
}

bool equal_var(Var *a, Var *b) // a and b can't both be constant
{
    return a == b ||
        (equal_parent(a->parent[0], b->parent[0]) && equal_parent(a->parent[1], b->parent[1])) ||
        (equal_parent(a->parent[0], b->parent[1]) && equal_parent(a->parent[1], b->parent[0]));
}

char *val_to_const(int val)
{
    bool neg = val < 0;
    size_t len = mlg10(abs(val)) + neg;
    char *s = (char *)malloc(sizeof(char) * (len + 2));
    for (size_t i = 0; i < len; i++)
    {
        s[i] = val % 10 + '0';
        val /= 10;
    }
    if (neg)
        s[len - 1] = '-';
    s[len] = '#';
    s[len + 1] = '\0';
    reverse_str(s, len + 1);
    return s;
}

int calc(int a, int b, char op)
{
    switch (op)
    {
        case '+':
            return a + b;
        case '-':
            return a - b;
        case '*':
            return a * b;
        case '/':
            return a / b;
    }
}

char *get_var_name(char type, size_t idx)
{
    size_t len = mlg10(idx);
    char *s = (char *)malloc(sizeof(char) * (len + 2));
    for (size_t i = 0; i < len; i++)
    {
        s[i] = idx % 10 + '0';
        idx /= 10;
    }
    s[len] = 'type';
    s[len + 1] = '\0';
    reverse_str(s, len + 1);
    return s;
}

void find_same(IR_list *ir)
{
    Var *x = get_var(ir->ss[0]);
    char type = 't';
    size_t idx;
    for (idx = 0; idx < tmp_cnt; idx++)
        if (&t_var[idx] != x && equal_var(&t_var[idx], x))
            break;
    if (idx < tmp_cnt)
    {
        
        return;
    }

    type = 'v';
    for (idx = 0; idx < tmp_cnt; idx++)
        if (&v_var[idx] != x && equal_var(&v_var[idx], x))
            break;
    if (idx < var_cnt)
    {

        return;
    }
}

void simplify_IR(IR_list *ir)
{
    debug_IR_list(ir);
    Var *x = get_var(ir->ss[0]), *y, *z;
    x->recent_ir = ir;
    switch (get_list_len(ir))
    {
        // (*)x := (*&)y
        case 3:
            if (ir->ss[2][0] == '&' || ir->ss[2][0] == '*')
                break;
            y = get_var(ir->ss[2]);
            if (y->type == CONST)
            {
                ir->ss[2] = val_to_const(y->val);
            }
            if (ir->ss[0][0] != '*')
            {
                x->parent[0] = (Parent){y, y->recent_ir};
                x->parent[1] = (Parent){NULL, NULL};
                x->type = y->type;
                x->val = y->val;
            }
            break;
        // x := y ? z
        case 5:
            y = get_var(ir->ss[2]), z = get_var(ir->ss[4]);
            char op = ir->ss[3][0];
            x->parent[0] = (Parent){y, y->recent_ir};
            x->parent[1] = (Parent){z, z->recent_ir};
            // #?
            if (y->type == CONST && z->type == CONST)
            {
                x->type = CONST;
                x->val = calc(y->val, z->val, op);
                for (size_t i = 2; i <= 4; i++)
                {
                    free(ir->ss[i]);
                    ir->ss[i] = NULL;
                }
                ir->ss[2] = val_to_const(x->val);
            }
            // #0
            else if ((op == '*' && ((y->type == CONST && y->val == 0) || (z->type == CONST && z->val == 0))) ||
                     (op == '-' && equal_var(y, z)) ||
                     (op == '/' && y->type == CONST && y->val == 0))
            {
                x->type = true;
                x->val = 0;
                for (size_t i = 2; i <= 4; i++)
                {
                    free(ir->ss[i]);
                    ir->ss[i] = NULL;
                }
                ir->ss[2] = val_to_const(0);
            }
            break;
    }
}

char opt_exp(IR_list *u)
{
    memset(v_var, 0, sizeof(v_var));
    memset(t_var, 0, sizeof(t_var));
    IR_list *ir = u, *tail;

    // pos: simplify const ops
    while (ir != NULL)
    {
        if (strcmp(ir->ss[1], ":=") == 0)
        {
            if (strcmp(ir->ss[2], "CALL") == 0);
            else
                simplify_IR(ir);
        }

        if (ir->next == NULL)
            tail = ir;
        ir = ir->next;
    }

    // neg: remove useless vars
    // ir = tail;
    // while (ir != NULL)
    // {
    //     if (strcmp(ir->ss[1], ":=") == 0);
    //     else if (strcmp(ir->ss[0], "LABEL") == 0);
    //     else if (strcmp(ir->ss[0], "FUNCTION") == 0);
    //     else if (strcmp(ir->ss[0], "GOTO") == 0);
    //     else if (strcmp(ir->ss[0], "IF") == 0);
    //     else if (strcmp(ir->ss[0], "RETURN") == 0);
    //     else if (strcmp(ir->ss[0], "DEC") == 0);
    //     else if (strcmp(ir->ss[0], "PARAM") == 0);
    //     else if (strcmp(ir->ss[0], "ARG") == 0);
    //     else if (strcmp(ir->ss[0], "READ") == 0);
    //     else if (strcmp(ir->ss[0], "WRITE") == 0)
    //     ir = ir->prev;
    // }
    
    return 0;
}

void output_list(const IR_list *u, FILE *fout)
{
    while (u != NULL)
    {
        for (int i = 0; u->ss[i] != NULL; ++i)
        {
            fprintf(fout, "%s ", u->ss[i]);
        }
        fprintf(fout, "\n");
        u = u->next;
    }
}

void optimize(FILE *fin, FILE *fout)
{
    IR_list *rootw = NULL, *nowp;
    while (fscanf(fin, "%[^\n]", s) != EOF)
    {
        fscanf(fin, "%*c");
        split_str(s); // similar to ss = s.split(whitespace), ss : list
        if (rootw == NULL)
        {
            rootw = new_IR_list(NULL);
            nowp = rootw;
        }
        else
        {
            nowp = new_IR_list(nowp);
        }
    }

    while (1)
    {
        if (opt_exp(rootw))
        {
            continue;
        }
        if (opt_if(rootw))
        {
            continue;
        }
        break;
    }

    output_list(rootw, fout);

    // if (strncmp(s, "LABEL", 5) == 0)
    // {
    //     // LABEL
    // }
    // else if (strncmp(s, "GOTO", 4) == 0)
    // {
    //     // GOTO
    // }
    // else if (strncmp(s, "IF", 2) == 0)
    // {
    //     // IF
    // }
    // else if (strncmp(s, "RETURN", 6) == 0)
    // {
    //     // RETURN
    // }
    // else if (strncmp(s, "DEC", 3) == 0)
    // {
    //     // DEC
    // }
    // else if (strncmp(s, "PARAM", 5) == 0)
    // {
    //     // PARAM
    // }
    // else if (strncmp(s, "ARG", 3) == 0)
    // {
    //     // ARG
    // }
    // else if (strncmp(s, "READ", 4) == 0)
    // {
    //     // READ
    // }
    // else if (strncmp(s, "WRITE", 5) == 0)
    // {
    //     // WRITE
    // }
    // else
    // {
    //     // :=
    // }
}

int main(int argc, char **argv)
{
    if (argc < 6)
    {
        fprintf(stderr, "useage: %s <in_file> <out_file> <label_cnt> <tmp_cnt> <var_cnt>\n", argv[0]);
        exit(1);
    }
    FILE *fin = fopen(argv[1], "r");
    FILE *fout = fopen(argv[2], "w");
    // fprintf(debug, "%s %s %s %s %s\n", argv[1], argv[2], argv[3], argv[4], argv[5]);
    debug = fopen("./test-ex/debug_info.txt", "w");
    printf("optimizing:\n");
    if (fin == NULL || fout == NULL)
    {
        fprintf(stderr, "Open file error\n");
        exit(0);
    }
    label_cnt = atoi(argv[3]);
    tmp_cnt = atoi(argv[4]);
    var_cnt = atoi(argv[5]);
    optimize(fin, fout);
    return 0;
}