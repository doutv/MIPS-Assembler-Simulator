// #define DEBUG_ASS
// #define DEBUG_DATA
#define DEBUG_SIM

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <functional>
#include <bitset>
#include <iostream>
#include <algorithm>
#include <bitset>
#include <string>
#include <cstring>
#include <vector>
#include <unordered_map>
#include <fstream>
using namespace std;

class Assembler
{
public:
    vector<string> data_seg;
    vector<string> text_seg;
    vector<string> output;
    class Scanner
    {
    public:
        Assembler &assembler;
        vector<string> file;

        void get_assembly(istream &in);
        void remove_comments();
        void split_data_and_text();
        void preprocess_text();
        vector<string> &get_data_seg();
        vector<string> &get_text_seg();
        void scan(istream &in);
        Scanner(Assembler &assembler) : assembler(assembler) {}
    };
    class Parser
    {
    public:
        Assembler &assembler;
        enum optype
        {
            O_type,
            R_type,
            I_type,
            J_type,
            P_type
        };
        unordered_map<string, uint32_t> label_to_addr;
        uint32_t pc = 0x400000;
        string *cur_string;
        size_t cur_string_idx;

        string get_R_instruction(const string &op);
        string get_I_instruction(const string &op);
        string get_J_instruction(const string &op);
        string get_O_instruction(const string &op);
        string get_next_token();
        string get_register_code(const string &r);
        string get_ascii_data(const string &data);
        int get_op_type(const string &op);
        string zero_extent(const string &s, const size_t target);
        void process_dataseg();
        void find_label();
        void parse();
        void print_machine_code(ostream &out);
        Parser(Assembler &assembler) : assembler(assembler) {}
    };
    Scanner scanner;
    Parser parser;
    Assembler() : scanner(*this), parser(*this) {}
};

void Assembler::Scanner::scan(istream &in)
{
    get_assembly(in);
    remove_comments();
    split_data_and_text();
    preprocess_text();
#ifdef DEBUG_ASS
    // cout << "---data seg---" << endl;
    // for (string &s : assembler.data_seg)
    //     cout << s << endl;
    // cout << "---end data seg---" << endl;
    // cout << "---text seg---" << endl;
    // for (string &s : assembler.text_seg)
    //     cout << s << endl;
    // cout << "---end text seg---" << endl;
#endif
}
void Assembler::Scanner::get_assembly(istream &in)
{
    string s;
    while (getline(in, s))
        file.push_back(s);
}
void Assembler::Scanner::remove_comments()
{
    /*
    remove comments and empty lines
    */
    vector<string> new_file;
    for (string &s : file)
    {
        if (s.find('#') != string::npos)
            s.erase(s.find('#'), s.size());
        if (s.find_first_not_of(' ') == string::npos)
            continue;
        if (!s.empty())
            new_file.push_back(s);
    }
    file = new_file;
}
void Assembler::Scanner::split_data_and_text()
{
    /*
    split data and text part
    .data 和 .text 可能交错出现
    */
    size_t i;
    for (i = 0; i < file.size(); i++)
    {
        string s = file[i];
        if (s.find(".text") != string::npos)
        {
            for (++i; i < file.size(); i++)
            {
                if (file[i].find(".data") != string::npos)
                    break;
                assembler.text_seg.push_back(file[i]);
            }
            --i;
            continue;
        }
        if (s.find(".data") != string::npos)
        {
            for (++i; i < file.size(); i++)
            {
                if (file[i].find(".text") != string::npos)
                    break;
                assembler.data_seg.push_back(file[i]);
            }
            --i;
            continue;
        }
    }
}
void Assembler::Scanner::preprocess_text()
{
    /*
    将 label 和指令放在同一行
    将 ',' 替换成空格' '
    去除 \t tab
    */
    vector<string> new_text_seg;
    string s;
    for (size_t i = 0; i < assembler.text_seg.size(); i++)
    {
        s = assembler.text_seg[i];
        if (s.find(':') != string::npos)
        {
            // locate label
            if (s.find_last_not_of(' ') == s.find(':'))
            {
                // 当前行只有 label:
                s = assembler.text_seg[i].substr(0, s.find(':') + 1);
                s += assembler.text_seg.at(i + 1);
                new_text_seg.push_back(s);
                ++i;
            }
            else
                new_text_seg.push_back(s);
        }
        else
            new_text_seg.push_back(s);
    }
    for (string &s : new_text_seg)
    {
        replace(s.begin(), s.end(), ',', ' ');
        s.erase(remove(s.begin(), s.end(), '\t'), s.end());
    }
    assembler.text_seg = new_text_seg;
}
string Assembler::Parser::get_next_token()
{
    string s = *cur_string;
    size_t st_idx = s.find_first_not_of(' ', cur_string_idx);
    size_t end_idx = s.find(' ', st_idx) == string::npos ? s.size() : s.find(' ', st_idx);
    string res = s.substr(st_idx, end_idx - st_idx);
    cur_string_idx = end_idx;
    return res;
}
string Assembler::Parser::get_ascii_data(const string &data)
{
#ifdef DEBUG_DATA
    cout << data << endl;
#endif
    string res;
    for (size_t i = 0; i < data.size(); i++)
    {
        if (data[i] == '\\' && i + 1 < data.size())
        {
            switch (data[i + 1])
            {
            case 'n':
                res += bitset<8>((uint8_t)('\n')).to_string();
                break;
            case 't':
                res += bitset<8>((uint8_t)('\t')).to_string();
                break;
            case '\'':
                res += bitset<8>((uint8_t)('\'')).to_string();
                break;
            case '\"':
                res += bitset<8>((uint8_t)('\"')).to_string();
                break;
            case '\\':
                res += bitset<8>((uint8_t)('\\')).to_string();
                break;
            default:
                break;
            }
        }
        else
        {
            res += bitset<8>((uint8_t)(data[i])).to_string();
        }
    }
    return res;
}
void Assembler::Parser::process_dataseg()
{
#ifndef DEBUG_ASS
    assembler.output.push_back(".data");
#endif
    for (string &s : assembler.data_seg)
    {
        string target_str, tmp;
        size_t st_idx, end_idx;
        if (s.find(".asciiz") != string::npos)
        {
            // Store the string str in memory and null- terminate it.
            target_str = ".asciiz";
            st_idx = s.find(target_str) + target_str.size();
            st_idx = s.find('\"', st_idx) + 1;
            end_idx = s.find('\"', st_idx);
            tmp = s.substr(st_idx, end_idx - st_idx);
            // add \0 terminator
            tmp = get_ascii_data(tmp) + bitset<8>(static_cast<unsigned long long>('\0')).to_string();
            assembler.output.push_back(tmp);
        }
        else if (s.find(".ascii") != string::npos)
        {
            // Store the string str in memory, but do not nullterminate it.
            target_str = ".ascii";
            st_idx = s.find(target_str) + target_str.size();
            st_idx = s.find('\"', st_idx) + 1;
            end_idx = s.find('\"', st_idx);
            tmp = s.substr(st_idx, end_idx - st_idx);
            assembler.output.push_back(get_ascii_data(tmp));
        }
        else if (s.find(".word") != string::npos)
        {
            target_str = ".word";
            st_idx = s.find(target_str) + target_str.size();
            for (size_t i = st_idx; i < s.size(); i++)
            {
                if (s[i] == ' ' || s[i] == ',')
                    continue;
                st_idx = i;
                while (s[i] >= '0' && s[i] <= '9')
                {
                    ++i;
                }
                string numstr = s.substr(st_idx, i - st_idx);
                // word: 32bits
                assembler.output.push_back(zero_extent(numstr, 32));
            }
        }
        else if (s.find(".byte") != string::npos)
        {
            string res;
            target_str = ".byte";
            st_idx = s.find(target_str) + target_str.size();
            for (size_t i = st_idx; i < s.size(); i++)
            {
                if (s[i] == ' ' || s[i] == ',')
                    continue;
                st_idx = i;
                while (s[i] >= '0' && s[i] <= '9')
                {
                    ++i;
                }
                string numstr = s.substr(st_idx, i - st_idx);
                // byte: 8bits
                res += zero_extent(numstr, 8);
            }
            assembler.output.push_back(res);
        }
        else if (s.find(".half") != string::npos)
        {
            string res;
            target_str = ".half";
            st_idx = s.find(target_str) + target_str.size();
            for (size_t i = st_idx; i < s.size(); i++)
            {
                if (s[i] == ' ' || s[i] == ',')
                    continue;
                st_idx = i;
                while (s[i] >= '0' && s[i] <= '9')
                {
                    ++i;
                }
                string numstr = s.substr(st_idx, i - st_idx);
                // half: 16bits
                res += zero_extent(numstr, 16);
            }
            assembler.output.push_back(res);
        }
    }
    // 补0或者截断
    for (size_t i = 0; i < assembler.output.size(); i++)
    {
        string s = assembler.output[i];
        if (s.find(".data") != string::npos)
            continue;
        if (s.size() > 32)
        {
            while (s.size() > 32)
            {
                assembler.output.insert(assembler.output.begin() + i, s.substr(0, 32));
                ++i;
                s = s.substr(32, string::npos);
                if (s.size() <= 32)
                    assembler.output[i] = s;
            }
        }
        while (assembler.output[i].size() < 32)
            assembler.output[i].push_back('0');
    }
}
void Assembler::Parser::find_label()
{
    uint32_t label_pc = pc;
    for (string &s : assembler.text_seg)
    {
        size_t i = s.find(':') == string::npos ? 0 : s.find(':');
        if (i)
        {
            // locate label
            size_t st = 0;
            while (st < s.size() && s[st] == ' ')
                ++st;
            string label = s.substr(st, i - st);
            label_to_addr.emplace(label, label_pc);
        }
        label_pc += 4;
    }
#ifdef DEBUG_LABEL
    for (auto &it : label_to_addr)
    {
        cout << it.first << " " << hex << it.second << endl;
    }
#endif
}
void Assembler::Parser::print_machine_code(ostream &out)
{
    for (string &s : assembler.output)
        out << s << endl;
}
string Assembler::Parser::get_register_code(const string &r)
{
    if (r == "$0" || r == "$zero")
        return "00000";
    if (r == "$1" || r == "$at")
        return "00001";
    if (r == "$2" || r == "$v0")
        return "00010";
    if (r == "$3" || r == "$v1")
        return "00011";
    if (r == "$4" || r == "$a0")
        return "00100";
    if (r == "$5" || r == "$a1")
        return "00101";
    if (r == "$6" || r == "$a2")
        return "00110";
    if (r == "$7" || r == "$a3")
        return "00111";
    if (r == "$8" || r == "$t0")
        return "01000";
    if (r == "$9" || r == "$t1")
        return "01001";
    if (r == "$10" || r == "$t2")
        return "01010";
    if (r == "$11" || r == "$t3")
        return "01011";
    if (r == "$12" || r == "$t4")
        return "01100";
    if (r == "$13" || r == "$t5")
        return "01101";
    if (r == "$14" || r == "$t6")
        return "01110";
    if (r == "$15" || r == "$t7")
        return "01111";
    if (r == "$16" || r == "$s0")
        return "10000";
    if (r == "$17" || r == "$s1")
        return "10001";
    if (r == "$18" || r == "$s2")
        return "10010";
    if (r == "$19" || r == "$s3")
        return "10011";
    if (r == "$20" || r == "$s4")
        return "10100";
    if (r == "$21" || r == "$s5")
        return "10101";
    if (r == "$22" || r == "$s6")
        return "10110";
    if (r == "$23" || r == "$s7")
        return "10111";
    if (r == "$24" || r == "$t8")
        return "11000";
    if (r == "$25" || r == "$t9")
        return "11001";
    if (r == "$26" || r == "$k0")
        return "11010";
    if (r == "$27" || r == "$k1")
        return "11011";
    if (r == "$28" || r == "$gp")
        return "11100";
    if (r == "$29" || r == "$sp")
        return "11101";
    if (r == "$30" || r == "$fp")
        return "11110";
    else
        return "11111";
}
int Assembler::Parser::get_op_type(const string &op)
{
    // 1 O
    if (op == "syscall")
        return O_type;
    // 39 R
    else if (op == "add" || op == "addu" || op == "sub" || op == "subu" ||
             op == "and" || op == "or" || op == "xor" || op == "nor" ||
             op == "slt" || op == "sltu" || op == "sll" || op == "srl" ||
             op == "sra" || op == "sllv" || op == "srlv" || op == "srav" ||
             op == "jr" || op == "div" || op == "divu" || op == "mult" ||
             op == "multu" || op == "jalr" || op == "mtlo" || op == "mthi" ||
             op == "mfhi" || op == "mflo" || op == "mul" || op == "madd" ||
             op == "msub" || op == "maddu" || op == "msubu" || op == "teq" ||
             op == "tne" || op == "tge" || op == "tgeu" || op == "tlt" ||
             op == "tltu" || op == "clo" || op == "clz")
        return R_type;
    // 36 I
    else if (op == "addi" || op == "addiu" || op == "andi" || op == "ori" ||
             op == "xori" || op == "lui" || op == "lw" || op == "sw" ||
             op == "beq" || op == "bne" || op == "slti" || op == "sltiu" ||
             op == "lb" || op == "lbu" || op == "lh" || op == "lhu" ||
             op == "sb" || op == "sh" || op == "blez" || op == "bltz" ||
             op == "bgez" || op == "bgtz" || op == "lwl" || op == "lwr" ||
             op == "swl" || op == "swr" || op == "ll" || op == "sc" ||
             op == "bgezal" || op == "bltzal" || op == "teqi" || op == "tnei" ||
             op == "tgei" || op == "tgeiu" || op == "tlti" || op == "tltiu")
        return I_type;
    // 2 J
    else if (op == "j" || op == "jal")
        return J_type;
    else
        return P_type;
}
string Assembler::Parser::zero_extent(const string &s, const size_t target)
{
    /*
    convert string in decimal to string in binary and add zero to its head
    e.g. "4" -> "0"*(target-3)+"100"
    */
    string res;
    int32_t num = stoi(s);
    switch (target)
    {
    case 5:
        res = bitset<5>(num).to_string();
        break;
    case 16:
        res = bitset<16>(num).to_string();
        break;
    case 20:
        res = bitset<20>(num).to_string();
        break;
    case 26:
        res = bitset<26>(num).to_string();
        break;
    case 32:
        res = bitset<32>(num).to_string();
        break;
    default:
        break;
    }
    return res;
}
void Assembler::Parser::parse()
{
    process_dataseg();
    find_label();
#ifndef DEBUG_ASS
    assembler.output.push_back(".text");
#endif
    for (string &s : assembler.text_seg)
    {
        size_t i = s.find(':') == string::npos ? 0 : s.find(':') + 1;
        cur_string = &s;
        i = s.find_first_not_of(' ', i);
        size_t end_idx = s.find(' ', i) == string::npos ? s.size() : s.find(' ', i);
        string op = s.substr(i, end_idx - i);
        cur_string_idx = s.find(' ', i);
        switch (get_op_type(op))
        {
        case R_type:
            assembler.output.push_back(get_R_instruction(op));
            break;
        case I_type:
            assembler.output.push_back(get_I_instruction(op));
            break;
        case J_type:
            assembler.output.push_back(get_J_instruction(op));
            break;
        case O_type:
            assembler.output.push_back(get_O_instruction(op));
            break;
        default:
            break;
        }
        pc += 4;
    }
}
string Assembler::Parser::get_O_instruction(const string &op)
{
    string machine_code;
    if (op == "syscall")
        machine_code = "00000000000000000000000000001100";
    return machine_code;
}

string Assembler::Parser::get_R_instruction(const string &op)
{
    /*
        R-instruction:
        op rs rt rd shamt funct
        | 6bits | 5bits | 5bits | 5bits | 5bits | 6bits |
        1. op: operation code, all zeros for R-instructions.
        2. rs: the first register operand
        3. rt: the second register operand
        4. rd: the destination register
        5. shamt: shift amount. 0 when N/A
        Here, we go through how this instruction and its machine code corresponds to each other.
        Machine codes of other R-instructions are constructed through the same process. As for Iinstructions and J-instructions, I will not go through an example. The formats of these two types of
        instructions are:
        6. funct: function code, which is used to identify which R-instruction this is.
        The add instruction has the format:
        add $rd, $rs, $rt
        Therefore, for add $t0, $t1, $t2, we have:
        000000 01001 01010 01000 00000 100000
    */
    string opcode, rd, rs, rt, shamt, funct;
    string temp;
    opcode = "000000";
    // op rd rs rt
    if (op == "add" || op == "addu" || op == "sub" || op == "subu" ||
        op == "and" || op == "or" || op == "xor" || op == "nor" ||
        op == "slt" || op == "sltu" || op == "mul")
    {
        temp = get_next_token();
        rd = get_register_code(temp);

        temp = get_next_token();
        rs = get_register_code(temp);

        temp = get_next_token();
        rt = get_register_code(temp);

        shamt = "00000";

        if (op == "add")
            funct = "100000";
        else if (op == "addu")
            funct = "100001";
        else if (op == "sub")
            funct = "100010";
        else if (op == "subu")
            funct = "100011";
        else if (op == "and")
            funct = "100100";
        else if (op == "or")
            funct = "100101";
        else if (op == "xor")
            funct = "100110";
        else if (op == "nor")
            funct = "100111";
        else if (op == "slt")
            funct = "101010";
        else if (op == "sltu")
            funct = "101011";
        else if (op == "mul")
        {
            opcode = "011100";
            funct = "000010";
        }
    }

    // op rd rt rs
    else if (op == "sllv" || op == "srlv" || op == "srav")
    {
        temp = get_next_token();
        rd = get_register_code(temp);

        temp = get_next_token();
        rt = get_register_code(temp);

        temp = get_next_token();
        rs = get_register_code(temp);

        shamt = "00000";

        if (op == "sllv")
            funct = "000100";
        else if (op == "srlv")
            funct = "000110";
        else if (op == "srav")
            funct = "000111";
        else
            ;
    }

    // op rs rt
    else if (op == "mult" || op == "multu" || op == "div" || op == "divu" ||
             op == "madd" || op == "msub" || op == "maddu" || op == "msubu" ||
             op == "teq" || op == "tne" || op == "tge" || op == "tgeu" ||
             op == "tlt" || op == "tltu")
    {
        temp = get_next_token();
        rs = get_register_code(temp);

        temp = get_next_token();
        rt = get_register_code(temp);

        rd = "00000";

        shamt = "00000";

        if (op == "mult")
            funct = "011000";
        else if (op == "multu")
            funct = "011001";
        else if (op == "div")
            funct = "011010";
        else if (op == "divu")
            funct = "011011";
        else if (op == "madd")
        {
            opcode = "011100";
            funct = "000000";
        }
        else if (op == "msub")
        {
            opcode = "011100";
            funct = "000100";
        }
        else if (op == "maddu")
        {
            opcode = "011100";
            funct = "000001";
        }
        else if (op == "msubu")
        {
            opcode = "011100";
            funct = "000101";
        }
        else if (op == "teq")
        {
            funct = "110100";
        }
        else if (op == "tne")
        {
            funct = "110110";
        }
        else if (op == "tge")
        {
            funct = "110000";
        }
        else if (op == "tgeu")
        {
            funct = "110001";
        }
        else if (op == "tlt")
        {
            funct = "110010";
        }
        else if (op == "tltu")
        {
            funct = "110011";
        }
    }

    // op rs rd
    else if (op == "jalr")
    {
        temp = get_next_token();
        rs = get_register_code(temp);

        temp = get_next_token();
        rd = get_register_code(temp);

        shamt = "00000";
        rt = "00000";

        funct = "001001";
    }

    // op rd rt shamt
    else if (op == "sll" || op == "srl" || op == "sra")
    {
        rs = "00000";
        temp = get_next_token();
        rd = get_register_code(temp);
        temp = get_next_token();
        rt = get_register_code(temp);
        shamt = get_next_token();
        shamt = zero_extent(shamt, 5);

        if (op == "sll")
            funct = "000000";
        else if (op == "sra")
            funct = "000011";
        else if (op == "srl")
            funct = "000010";
        else
            ;
    }

    // op rs
    else if (op == "jr" || op == "mthi" || op == "mtlo")
    {
        temp = get_next_token();
        rs = get_register_code(temp);
        rt = "00000";
        rd = "00000";
        shamt = "00000";

        if (op == "mthi")
            funct = "010001";
        else if (op == "mtlo")
            funct = "010011";
        else if (op == "jr")
            funct = "001000";
        else
            ;
    }

    // op rd
    else if (op == "mfhi" || op == "mflo")
    {
        temp = get_next_token();
        rd = get_register_code(temp);
        rs = "00000";
        rt = "00000";
        shamt = "00000";

        if (op == "mfhi")
            funct = "010000";
        else if (op == "mflo")
            funct = "010010";
        else
            ;
    }

    // op rd rs
    else if (op == "clo" || op == "clz")
    {
        temp = get_next_token();
        rd = get_register_code(temp);
        temp = get_next_token();
        rs = get_register_code(temp);

        if (op == "clo")
            funct = "100001";
        else if (op == "clz")
            funct = "100000";
    }
    string machine_code = opcode + rs + rt + rd + shamt + funct;
    return machine_code;
}

string Assembler::Parser::get_I_instruction(const string &op)
{
    /*
        I-instruction:
        op rs rt immediate
        | 6bits | 5bits | 5bits | 16bits |
        1. op: the operation code that specifies which I-instruction this is.
        2. rs: register that contains the base address
        3. rt: the destination/source register (depends on the operation)
        4. immediate: a numerical value or offset (depends on the operation)
    */
    string opcode, rs, rt, imme;
    string temp;
    uint32_t addr;
    // op rs rt imme
    if (op == "beq" || op == "bne")
    {
        temp = get_next_token();
        rs = get_register_code(temp);

        temp = get_next_token();
        rt = get_register_code(temp);

        temp = get_next_token();
        if (label_to_addr.find(temp) != label_to_addr.end())
        {
            addr = label_to_addr[temp];
            addr = (addr - (pc + 4)) / 4; // compute offset
            imme = zero_extent(to_string(addr), 16);
        }
        else
            imme = zero_extent(temp, 16);

        if (op == "beq")
            opcode = "000100";
        else if (op == "bne")
            opcode = "000101";
    }

    // op rt ts imme
    else if (op == "addi" || op == "addiu" || op == "andi" || op == "ori" ||
             op == "xori" || op == "slti" || op == "slti" || op == "sltiu")
    {
        temp = get_next_token();
        rt = get_register_code(temp);
        temp = get_next_token();
        rs = get_register_code(temp);

        temp = get_next_token();

        if (op == "addi" || op == "slti" || op == "sltiu" || op == "addiu")
            imme = zero_extent(temp, 16);
        else
            imme = zero_extent(temp, 16);

        if (op == "addi")
            opcode = "001000";
        else if (op == "addiu")
            opcode = "001001";
        else if (op == "andi")
            opcode = "001100";
        else if (op == "ori")
            opcode = "001101";
        else if (op == "xori")
            opcode = "001110";
        else if (op == "slti")
            opcode = "001010";
        else if (op == "sltiu")
            opcode = "001011";
    }

    // op rt imme(rs)
    else if (op == "lw" || op == "sw" || op == "lb" || op == "lbu" || op == "lh" ||
             op == "lhu" || op == "sb" || op == "sb" || op == "sh" || op == "lwl" ||
             op == "lwr" || op == "swl" || op == "swr" || op == "ll" || op == "sc")
    {
        temp = get_next_token();
        rt = get_register_code(temp);

        // imme(rs)
        temp = get_next_token();
        imme = temp.substr(0, temp.find('('));
        imme = zero_extent(imme, 16);
        rs = temp.substr(temp.find('(') + 1, temp.find(')') - temp.find('(') - 1);
        rs = get_register_code(rs);

        if (op == "lw")
            opcode = "100011";
        else if (op == "sw")
            opcode = "101011";
        else if (op == "lb")
            opcode = "100000";
        else if (op == "lbu")
            opcode = "100100";
        else if (op == "lh")
            opcode = "100001";
        else if (op == "lhu")
            opcode = "100101";
        else if (op == "sb")
            opcode = "101000";
        else if (op == "sh")
            opcode = "101001";
        else if (op == "lwl")
            opcode = "100010";
        else if (op == "lwr")
            opcode = "100110";
        else if (op == "swl")
            opcode = "101010";
        else if (op == "swr")
            opcode = "101110";
        else if (op == "ll")
            opcode = "110000";
        else if (op == "sc")
            opcode = "111000";
    }

    // op rt imme
    else if (op == "lui")
    {
        temp = get_next_token();
        rt = get_register_code(temp);

        rs = "00000";

        temp = get_next_token();
        imme = zero_extent(temp, 16);

        opcode = "001111";
    }

    // op rs imme
    else if (op == "blez" || op == "bltz" || op == "bgtz" || op == "bgez" ||
             op == "bgezal" || op == "bltzal" || op == "teqi" || op == "tnei" ||
             op == "tgei" || op == "tgeiu" || op == "tlti" || op == "tltiu")

    {
        temp = get_next_token();
        rs = get_register_code(temp);

        temp = get_next_token();
        if (label_to_addr.find(temp) != label_to_addr.end())
        {
            addr = label_to_addr[temp];
            addr = (addr - (pc + 4)) / 4; // compute offset
            imme = zero_extent(to_string(addr), 16);
        }
        else
            imme = zero_extent(temp, 16);

        if (op == "bgez")
        {
            opcode = "000001";
            rt = "00001";
        }
        else if (op == "bgtz")
        {
            opcode = "000111";
            rt = "00000";
        }
        else if (op == "blez")
        {
            opcode = "000110";
            rt = "00000";
        }
        else if (op == "bltz")
        {
            opcode = "000001";
            rt = "00000";
        }
        else if (op == "bgezal")
        {
            opcode = "000001";
            rt = "10001";
        }
        else if (op == "bltzal")
        {
            opcode = "000001";
            rt = "10000";
        }
        else if (op == "teqi")
        {
            opcode = "000001";
            rt = "01100";
        }
        else if (op == "tnei")
        {
            opcode = "000001";
            rt = "01110";
        }
        else if (op == "tgei")
        {
            opcode = "000001";
            rt = "01000";
        }
        else if (op == "tgeiu")
        {
            opcode = "000001";
            rt = "01001";
        }
        else if (op == "tlti")
        {
            opcode = "000001";
            rt = "01010";
        }
        else if (op == "tltiu")
        {
            opcode = "000001";
            rt = "01011";
        }
    }
    string machine_code = opcode + rs + rt + imme;
    return machine_code;
}

string Assembler::Parser::get_J_instruction(const string &op)
{
    /*
        J-instruction:
        op address
        | 6bits | 26bits |
        1. op: the operation code that specifies which J-instruction this is.
        2. address: the address to jump to, usually associate with a label. Since the
        address of an instruction in the memory is always divisible by 4 (think about
        why), the last two bits are always zero, so the last two bits are dropped.
    */
    string opcode, target;
    uint32_t addr;
    if (op == "j")
        opcode = "000010";
    else if (op == "jal")
        opcode = "000011";
    else
        ;

    target = get_next_token();
    if (label_to_addr.find(target) != label_to_addr.end())
    {
        addr = label_to_addr[target];
        addr >>= 2;
        target = zero_extent(to_string(addr), 26);
    }
    else
        target = zero_extent(target, 26);

    string machine_code = opcode + target;
    return machine_code;
}

class Simulator
{
public:
    /*
    Memory structure:
    stack_st_idx = 6 * 1024 * 1024
    | <- stack data
    stack_end_idx
    | 
    dynamic_end_idx
    | <- dynamic data
    dynamic_st_idx = static_end_idx
    | <- static data
    static_st_idx = 1 * 1024 * 1024
    |
    text_end_idx
    | <- text data
    text_st_idx = 0
    */
    static const size_t word_size = 32;
    static const size_t half_size = 16;
    static const size_t byte_size = 8;
    typedef array<char, word_size> word_t;
    typedef array<char, half_size> half_t;
    typedef array<char, byte_size> byte_t;
    static const uint32_t base_vm = 0x400000;
    static const size_t memory_size = 6 * 1024 * 1024; // 6MB
    static array<byte_t, memory_size> memory;          // char memory[memory_size][8]
    static const size_t reg_size = 34;
    static int32_t reg[reg_size];
    static size_t text_end_idx;
    static const size_t static_st_idx = 1024 * 1024;
    static size_t dynamic_st_idx;
    static size_t dynamic_end_idx;
    static const size_t stack_end_idx = memory_size;
    static unordered_map<string, size_t> regcode_to_idx;
    const vector<string> &input;
    vector<string> output;
    static const size_t v0 = 2;
    static const size_t a0 = 4;
    static const size_t a1 = 5;
    static const size_t a2 = 6;
    static const size_t lo = 32;
    static const size_t hi = 33;
    static uint32_t pc;

    static void store_word_to_memory(const word_t &word, uint32_t addr);
    static void store_half_to_memory(const half_t &half, uint32_t addr);
    static void store_byte_to_memory(const byte_t &byte, uint32_t addr);
    static word_t get_word_from_memory(uint32_t addr);
    static half_t get_half_from_memory(uint32_t addr);
    static byte_t get_byte_from_memory(uint32_t addr);
    static int32_t get_wordval_from_memory(uint32_t addr);
    static int16_t get_halfval_from_memory(uint32_t addr);
    static int8_t get_byteval_from_memory(uint32_t addr);
    void gen_regcode_to_idx();
    void store_static_data();
    void init_reg_value();
    void store_text();
    void simulate();
    static size_t addr2idx(uint32_t vm);
    static size_t idx2addr(size_t idx);
    Simulator(vector<string> &input_) : input(input_) {}

    unordered_map<string, function<void(const string &)>> opcode_to_func;
    unordered_map<string, function<void(const string &)>> opcode_funct_to_func;
    unordered_map<string, function<void(const string &)>> rt_to_func;
    void exec_instr(const string &mc);
    void gen_opcode_to_func();
    void gen_opcode_funct_to_func();
    void gen_rt_to_func();
    static int32_t &get_regv(const string &reg_str);
    // lots of instruction functions
    static void signal_exception(const string &err)
    {
        cout << err << endl;
        exit(1);
    }
    static int32_t sign_extent(const string &imme)
    {
        int32_t imme_val = stoi(imme, nullptr, 2);
        int32_t sign_mask = 1 << 15;
        int32_t sign = imme_val & sign_mask;
        if (sign == sign_mask)
        {
            imme_val = imme_val | (((1 << 16) - 1) << 16);
        }
        return imme_val;
    }
    // R instructions
    static void instr_add(const string &mc)
    {
        string rs, rt, rd, shamt;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        rd = mc.substr(16, 5);
        shamt = mc.substr(21, 5);
        if (__builtin_add_overflow(get_regv(rs), get_regv(rt), &get_regv(rd)))
            signal_exception("overflow");
    }
    static void instr_addu(const string &mc)
    {
        string rs, rt, rd, shamt;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        rd = mc.substr(16, 5);
        shamt = mc.substr(21, 5);
        if (__builtin_add_overflow((uint32_t)get_regv(rs), (uint32_t)get_regv(rt), &get_regv(rd)))
            signal_exception("overflow");
    }
    static void instr_and(const string &mc)
    {
        string rs, rt, rd, shamt;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        rd = mc.substr(16, 5);
        shamt = mc.substr(21, 5);
        get_regv(rd) = get_regv(rs) & get_regv(rt);
    }
    static void instr_clo(const string &mc)
    {
        string rs, rt, rd, shamt;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        rd = mc.substr(16, 5);
        shamt = mc.substr(21, 5);
        size_t cnt = 0;
        for (size_t i = 31; i >= 0; i--)
        {
            if (get_regv(rs) & (1 << i) == 0)
            {
                cnt = 32 - i;
                break;
            }
        }
        get_regv(rd) = cnt;
    }
    static void instr_clz(const string &mc)
    {
        string rs, rt, rd, shamt;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        rd = mc.substr(16, 5);
        shamt = mc.substr(21, 5);
        size_t cnt = 0;
        for (size_t i = 31; i >= 0; i--)
        {
            if (get_regv(rs) & (1 << i))
            {
                cnt = 32 - i;
                break;
            }
        }
        get_regv(rd) = cnt;
    }
    static void instr_div(const string &mc)
    {
        string rs, rt, rd, shamt;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        rd = mc.substr(16, 5);
        shamt = mc.substr(21, 5);
        reg[lo] = get_regv(rs) / get_regv(rt);
        reg[hi] = get_regv(rs) / get_regv(rt);
    }
    static void instr_divu(const string &mc)
    {
        string rs, rt, rd, shamt;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        rd = mc.substr(16, 5);
        shamt = mc.substr(21, 5);
        reg[lo] = (uint32_t)get_regv(rs) / (uint32_t)get_regv(rt);
        reg[hi] = (uint32_t)get_regv(rs) / (uint32_t)get_regv(rt);
    }
    static void instr_mult(const string &mc)
    {
        string rs, rt, rd, shamt;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        rd = mc.substr(16, 5);
        shamt = mc.substr(21, 5);
        int64_t tmp = (int64_t)get_regv(rs) * (int64_t)get_regv(rt);
        reg[lo] = tmp & numeric_limits<int32_t>::max();
        reg[hi] = tmp >> 32;
    }
    static void instr_multu(const string &mc)
    {
        string rs, rt, rd, shamt;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        rd = mc.substr(16, 5);
        shamt = mc.substr(21, 5);
        uint64_t tmp = (uint64_t)get_regv(rs) * (uint64_t)get_regv(rt);
        reg[lo] = tmp & numeric_limits<uint32_t>::max();
        reg[hi] = tmp >> 32;
    }
    static void instr_mul(const string &mc)
    {
        string rs, rt, rd, shamt;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        rd = mc.substr(16, 5);
        shamt = mc.substr(21, 5);
        int64_t tmp = (int64_t)get_regv(rs) * (int64_t)get_regv(rt);
        get_regv(rd) = (int32_t)tmp;
    }
    static void instr_madd(const string &mc)
    {
        string rs, rt, rd, shamt;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        rd = mc.substr(16, 5);
        shamt = mc.substr(21, 5);
        int64_t tmp = (((int64_t)reg[hi] << 32) | reg[lo]) + ((int64_t)get_regv(rs) * get_regv(rt));
        reg[lo] = tmp & numeric_limits<int32_t>::max();
        reg[hi] = tmp >> 32;
    }
    static void instr_msub(const string &mc)
    {
        string rs, rt, rd, shamt;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        rd = mc.substr(16, 5);
        int64_t tmp = (((int64_t)reg[hi] << 32) | reg[lo]) - ((int64_t)get_regv(rs) * get_regv(rt));
        reg[lo] = tmp & numeric_limits<int32_t>::max();
        reg[hi] = tmp >> 32;
    }
    static void instr_maddu(const string &mc)
    {
        string rs, rt, rd, shamt;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        rd = mc.substr(16, 5);
        shamt = mc.substr(21, 5);
        uint64_t tmp = (((uint64_t)reg[hi] << 32) | reg[lo]) + ((uint64_t)get_regv(rs) * get_regv(rt));
        reg[lo] = tmp & numeric_limits<uint32_t>::max();
        reg[hi] = tmp >> 32;
    }
    static void instr_msubu(const string &mc)
    {
        string rs, rt, rd, shamt;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        rd = mc.substr(16, 5);
        shamt = mc.substr(21, 5);
        uint64_t tmp = (((uint64_t)reg[hi] << 32) | reg[lo]) - ((uint64_t)get_regv(rs) * get_regv(rt));
        reg[lo] = tmp & numeric_limits<uint32_t>::max();
        reg[hi] = tmp >> 32;
    }
    static void instr_nor(const string &mc)
    {
        string rs, rt, rd, shamt;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        rd = mc.substr(16, 5);
        shamt = mc.substr(21, 5);
        get_regv(rd) = ~(get_regv(rs) | get_regv(rt));
    }
    static void instr_or(const string &mc)
    {
        string rs, rt, rd, shamt;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        rd = mc.substr(16, 5);
        shamt = mc.substr(21, 5);
        get_regv(rd) = (get_regv(rs) | get_regv(rt));
    }
    static void instr_sll(const string &mc)
    {
        string rs, rt, rd, shamt;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        rd = mc.substr(16, 5);
        shamt = mc.substr(21, 5);
        int32_t shamt_val = stoi(shamt, nullptr, 2);
        get_regv(rd) = get_regv(rt) << shamt_val;
    }
    static void instr_sllv(const string &mc)
    {
        string rs, rt, rd, shamt;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        rd = mc.substr(16, 5);
        shamt = mc.substr(21, 5);
        get_regv(rd) = get_regv(rt) << get_regv(rs);
    }
    static void instr_sra(const string &mc)
    {
        // arithmetic shift
        string rs, rt, rd, shamt;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        rd = mc.substr(16, 5);
        shamt = mc.substr(21, 5);
        int32_t shamt_val = stoi(shamt, nullptr, 2);
        get_regv(rd) = get_regv(rt) >> shamt_val;
    }
    static void instr_srav(const string &mc)
    {
        string rs, rt, rd, shamt;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        rd = mc.substr(16, 5);
        shamt = mc.substr(21, 5);
        get_regv(rd) = get_regv(rt) >> get_regv(rs);
    }
    static void instr_srl(const string &mc)
    {
        string rs, rt, rd, shamt;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        rd = mc.substr(16, 5);
        shamt = mc.substr(21, 5);
        int32_t shamt_val = stoi(shamt, nullptr, 2);
        get_regv(rd) = (get_regv(rt) >> shamt_val) & ((1 << (32 - shamt_val)) - 1);
    }
    static void instr_srlv(const string &mc)
    {
        string rs, rt, rd, shamt;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        rd = mc.substr(16, 5);
        shamt = mc.substr(21, 5);
        int32_t shamt_val = get_regv(rs) & (0b11111);
        get_regv(rd) = (get_regv(rt) >> shamt_val) & ((1 << (32 - shamt_val)) - 1);
    }
    static void instr_sub(const string &mc)
    {
        string rs, rt, rd, shamt;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        rd = mc.substr(16, 5);
        shamt = mc.substr(21, 5);
        if (__builtin_sub_overflow(get_regv(rs), get_regv(rt), &get_regv(rd)))
            signal_exception("overflow");
    }
    static void instr_subu(const string &mc)
    {
        string rs, rt, rd, shamt;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        rd = mc.substr(16, 5);
        shamt = mc.substr(21, 5);
        get_regv(rd) = (uint32_t)get_regv(rs) - (uint32_t)get_regv(rt);
    }
    static void instr_xor(const string &mc)
    {
        string rs, rt, rd, shamt;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        rd = mc.substr(16, 5);
        shamt = mc.substr(21, 5);
        get_regv(rd) = get_regv(rs) ^ get_regv(rt);
    }
    static void instr_slt(const string &mc)
    {
        string rs, rt, rd, shamt;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        rd = mc.substr(16, 5);
        shamt = mc.substr(21, 5);
        get_regv(rd) = (get_regv(rs) < get_regv(rt)) ? 1 : 0;
    }
    static void instr_sltu(const string &mc)
    {
        string rs, rt, rd, shamt;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        rd = mc.substr(16, 5);
        shamt = mc.substr(21, 5);
        get_regv(rd) = ((uint32_t)get_regv(rs) < (uint32_t)get_regv(rt)) ? 1 : 0;
    }
    static void instr_jalr(const string &mc)
    {
        string rs, rt, rd, shamt;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        rd = mc.substr(16, 5);
        shamt = mc.substr(21, 5);
        get_regv(rd) = pc;
        pc = get_regv(rs);
    }
    static void instr_jr(const string &mc)
    {
        string rs, rt, rd, shamt;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        rd = mc.substr(16, 5);
        shamt = mc.substr(21, 5);
        pc = get_regv(rs);
    }
    static void instr_teq(const string &mc)
    {
        string rs, rt, rd, shamt;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        rd = mc.substr(16, 5);
        shamt = mc.substr(21, 5);
        if (get_regv(rs) == get_regv(rt))
        {
            signal_exception("Trap");
        }
    }
    static void instr_tne(const string &mc)
    {
        string rs, rt, rd, shamt;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        rd = mc.substr(16, 5);
        shamt = mc.substr(21, 5);
        if (get_regv(rs) != get_regv(rt))
        {
            signal_exception("Trap if not equal");
        }
    }
    static void instr_tge(const string &mc)
    {
        string rs, rt, rd, shamt;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        rd = mc.substr(16, 5);
        shamt = mc.substr(21, 5);
        if (get_regv(rs) >= get_regv(rt))
        {
            signal_exception("Trap if greater or equal");
        }
    }
    static void instr_tgeu(const string &mc)
    {
        string rs, rt, rd, shamt;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        rd = mc.substr(16, 5);
        shamt = mc.substr(21, 5);
        if (get_regv(rs) >= (uint32_t)get_regv(rt))
        {
            signal_exception("Trap if greater or equal unsigned");
        }
    }
    static void instr_tlt(const string &mc)
    {
        string rs, rt, rd, shamt;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        rd = mc.substr(16, 5);
        shamt = mc.substr(21, 5);
        if (get_regv(rs) < get_regv(rt))
        {
            signal_exception("Trap if less than");
        }
    }
    static void instr_tltu(const string &mc)
    {
        string rs, rt, rd, shamt;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        rd = mc.substr(16, 5);
        shamt = mc.substr(21, 5);
        if ((uint32_t)get_regv(rs) < (uint32_t)get_regv(rt))
        {
            signal_exception("Trap if less than unsigned");
        }
    }
    static void instr_mfhi(const string &mc)
    {
        string rs, rt, rd, shamt;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        rd = mc.substr(16, 5);
        shamt = mc.substr(21, 5);
        get_regv(rd) = reg[hi];
    }
    static void instr_mflo(const string &mc)
    {
        string rs, rt, rd, shamt;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        rd = mc.substr(16, 5);
        shamt = mc.substr(21, 5);
        get_regv(rd) = reg[lo];
    }
    static void instr_mthi(const string &mc)
    {
        string rs, rt, rd, shamt;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        rd = mc.substr(16, 5);
        shamt = mc.substr(21, 5);
        reg[hi] = get_regv(rs);
    }
    static void instr_mtlo(const string &mc)
    {
        string rs, rt, rd, shamt;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        rd = mc.substr(16, 5);
        shamt = mc.substr(21, 5);
        reg[lo] = get_regv(rs);
    }

    // I instructions
    static void instr_addi(const string &mc)
    {
        string rs, rt, rd, shamt;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        rd = mc.substr(16, 5);
        shamt = mc.substr(21, 5);
        cout << "addi:" << mc << endl;
    }
    static void instr_addiu(const string &mc)
    {
        string rs, rt, rd, shamt;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        rd = mc.substr(16, 5);
        shamt = mc.substr(21, 5);
    }
    static void instr_andi(const string &mc)
    {
        string rs, rt, rd, shamt;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        rd = mc.substr(16, 5);
        shamt = mc.substr(21, 5);
    }
    static void instr_ori(const string &mc)
    {
        string rs, rt, imme;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        imme = mc.substr(16, 16);
        int32_t imme_val = sign_extent(imme);
        get_regv(rt) = ~(get_regv(rs) | imme_val);
    }
    static void instr_xori(const string &mc)
    {
        string rs, rt, imme;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        imme = mc.substr(16, 16);
        int32_t imme_val = sign_extent(imme);
        get_regv(rt) = get_regv(rs) ^ imme_val;
    }
    static void instr_lui(const string &mc)
    {
        string rs, rt, imme;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        imme = mc.substr(16, 16);
        int32_t imme_val = sign_extent(imme);
        get_regv(rt) = imme_val << 16;
    }
    static void instr_slti(const string &mc)
    {
        string rs, rt, imme;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        imme = mc.substr(16, 16);
        int32_t imme_val = sign_extent(imme);
        get_regv(rt) = (get_regv(rs) < imme_val) ? 1 : 0;
    }
    static void instr_sltiu(const string &mc)
    {
        string rs, rt, imme;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        imme = mc.substr(16, 16);
        int32_t imme_val = sign_extent(imme);
        get_regv(rt) = ((uint32_t)get_regv(rs) < (uint32_t)imme_val) ? 1 : 0;
    }
    static void instr_beq(const string &mc)
    {
        string rs, rt, imme;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        imme = mc.substr(16, 16);
        int32_t offset = sign_extent(imme);
        offset <<= 2;
        if (get_regv(rs) == get_regv(rt))
            pc += offset;
    }
    static void instr_bgez(const string &mc)
    {
        string rs, rt, imme;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        imme = mc.substr(16, 16);
        int32_t offset = sign_extent(imme);
        offset <<= 2;
        if (get_regv(rs) >= 0)
            pc += offset;
    }
    static void instr_bgezal(const string &mc)
    {
        string rs, rt, imme;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        imme = mc.substr(16, 16);
        int32_t offset = sign_extent(imme);
        offset <<= 2;
        reg[31] = pc + 4;
        if (get_regv(rs) >= 0)
            pc += offset;
    }
    static void instr_bgtz(const string &mc)
    {
        string rs, rt, imme;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        imme = mc.substr(16, 16);
        int32_t offset = sign_extent(imme);
        offset <<= 2;
        if (get_regv(rs) > 0)
            pc += offset;
    }
    static void instr_blez(const string &mc)
    {
        string rs, rt, imme;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        imme = mc.substr(16, 16);
        int32_t offset = sign_extent(imme);
        offset <<= 2;
        if (get_regv(rs) <= 0)
            pc += offset;
    }
    static void instr_bltzal(const string &mc)
    {
        string rs, rt, imme;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        imme = mc.substr(16, 16);
        int32_t offset = sign_extent(imme);
        offset <<= 2;
        if (get_regv(rs) < 0)
            pc += offset;
    }
    static void instr_bltz(const string &mc)
    {
        string rs, rt, imme;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        imme = mc.substr(16, 16);
        int32_t offset = sign_extent(imme);
        offset <<= 2;
        if (get_regv(rs) < 0)
            pc += offset;
    }
    static void instr_bne(const string &mc)
    {
        string rs, rt, imme;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        imme = mc.substr(16, 16);
        int32_t offset = sign_extent(imme);
        offset <<= 2;
        if (get_regv(rs) != get_regv(rt))
            pc += offset;
    }

    static void instr_teqi(const string &mc)
    {
        string rs, rt, imme;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        imme = mc.substr(16, 16);
        int32_t imme_val = sign_extent(imme);
        if (get_regv(rs) == imme_val)
        {
            signal_exception("Trap if equal immediate");
        }
    }
    static void instr_tnei(const string &mc)
    {
        string rs, rt, imme;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        imme = mc.substr(16, 16);
        int32_t imme_val = sign_extent(imme);
        if (get_regv(rs) != imme_val)
        {
            signal_exception("Trap if equal immediate");
        }
    }
    static void instr_tgei(const string &mc)
    {
        string rs, rt, imme;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        imme = mc.substr(16, 16);
        int32_t imme_val = sign_extent(imme);
        if (get_regv(rs) >= imme_val)
        {
            signal_exception("Trap if greater or equal");
        }
    }
    static void instr_tgeiu(const string &mc)
    {
        string rs, rt, imme;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        imme = mc.substr(16, 16);
        int32_t imme_val = sign_extent(imme);
        if (get_regv(rs) >= (uint32_t)imme_val)
        {
            signal_exception("Trap if greater or equal unsigned");
        }
    }
    static void instr_tlti(const string &mc)
    {
        string rs, rt, imme;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        imme = mc.substr(16, 16);
        int32_t imme_val = sign_extent(imme);
        if (get_regv(rs) < imme_val)
        {
            signal_exception("Trap if less than immediate");
        }
    }
    static void instr_tltiu(const string &mc)
    {
        string rs, rt, imme;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        imme = mc.substr(16, 16);
        int32_t imme_val = sign_extent(imme);
        if (get_regv(rs) < (uint32_t)imme_val)
        {
            signal_exception("Trap if less than immediate");
        }
    }
    static void instr_lb(const string &mc)
    {
        string rs, rt, imme;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        imme = mc.substr(16, 16);
        int32_t imme_val = sign_extent(imme);
        int32_t byte_val = get_byteval_from_memory(get_regv(rs) + imme_val);
        int32_t mask = 1 << 7;
        bool sign = (byte_val & mask == mask) ? 1 : 0;
        get_regv(rt) = sign ? (((1 << 24) - 1) << 8) & byte_val : byte_val;
    }
    static void instr_lbu(const string &mc)
    {
        string rs, rt, imme;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        imme = mc.substr(16, 16);
        int32_t imme_val = sign_extent(imme);
        get_regv(rt) = get_byteval_from_memory(get_regv(rs) + imme_val);
    }
    static void instr_lh(const string &mc)
    {
        string rs, rt, imme;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        imme = mc.substr(16, 16);
        int32_t imme_val = sign_extent(imme);
        get_regv(rt) = get_halfval_from_memory(get_regv(rs) + imme_val);
    }
    static void instr_lhu(const string &mc)
    {
        string rs, rt, imme;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        imme = mc.substr(16, 16);
        int32_t imme_val = sign_extent(imme);
        int32_t tmp = get_byteval_from_memory(get_regv(rs) + imme_val);
        get_regv(rt) = tmp & ((1 << 16) - 1);
    }
    static void instr_lw(const string &mc)
    {
        string rs, rt, imme;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        imme = mc.substr(16, 16);
        int32_t imme_val = sign_extent(imme);
        get_regv(rt) = get_wordval_from_memory(get_regv(rs) + imme_val);
    }
    static void instr_lwl(const string &mc)
    {
        string rs, rt, imme;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        imme = mc.substr(16, 16);
    }
    static void instr_lwr(const string &mc)
    {
        string rs, rt, imme;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        imme = mc.substr(16, 16);
    }
    static void instr_ll(const string &mc)
    {
        string rs, rt, imme;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        imme = mc.substr(16, 16);
        int32_t imme_val = sign_extent(imme);
        get_regv(rt) = get_wordval_from_memory(get_regv(rs) + imme_val);
    }
    static void instr_sb(const string &mc)
    {
        string rs, rt, imme;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        imme = mc.substr(16, 16);
        int32_t imme_val = sign_extent(imme);
        byte_t byte;
        string byte_str = bitset<byte_size>(get_regv(rt)).to_string();
        copy(byte_str.begin(), byte_str.end(), byte.data());
        store_byte_to_memory(byte, get_regv(rs) + imme_val);
    }
    static void instr_sh(const string &mc)
    {
        string rs, rt, imme;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        imme = mc.substr(16, 16);
        int32_t imme_val = sign_extent(imme);
        half_t half;
        string half_str = bitset<half_size>(get_regv(rt)).to_string();
        copy(half_str.begin(), half_str.end(), half.data());
        store_half_to_memory(half, get_regv(rs) + imme_val);
    }
    static void instr_sw(const string &mc)
    {
        string rs, rt, imme;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        imme = mc.substr(16, 16);
        int32_t imme_val = sign_extent(imme);
        word_t word;
        string word_str = bitset<word_size>(get_regv(rt)).to_string();
        copy(word_str.begin(), word_str.end(), word.data());
        store_word_to_memory(word, get_regv(rs) + imme_val);
    }
    static void instr_swl(const string &mc)
    {
        string rs, rt, imme;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        imme = mc.substr(16, 16);
    }
    static void instr_swr(const string &mc)
    {
        string rs, rt, imme;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        imme = mc.substr(16, 16);
    }
    static void instr_sc(const string &mc)
    {
        string rs, rt, imme;
        rs = mc.substr(6, 5);
        rt = mc.substr(11, 5);
        imme = mc.substr(16, 16);
        int32_t imme_val = sign_extent(imme);
        word_t word;
        string word_str = bitset<word_size>(get_regv(rt)).to_string();
        copy(word_str.begin(), word_str.end(), word.data());
        store_word_to_memory(word, get_regv(rs) + imme_val);
    }

    // J instructions
    static void instr_j(const string &mc)
    {
        string imme;
        imme = mc.substr(6, 26);
        int32_t imme_val = sign_extent(imme);
        int32_t pc_hi4 = pc & (((1 << 4) - 1) << 28);
        pc = pc_hi4 | ((imme_val << 2) & ((1 << 28) - 1));
    }
    static void instr_jal(const string &mc)
    {
        string imme;
        imme = mc.substr(6, 26);
        reg[31] = pc;
        int32_t imme_val = sign_extent(imme);
        int32_t pc_hi4 = pc & (((1 << 4) - 1) << 28);
        pc = pc_hi4 | ((imme_val << 2) & ((1 << 28) - 1));
    }

    // O instructions
    static void instr_syscall(const string &mc)
    {
        switch (reg[v0])
        {
        case 1: // print_int
        {
            printf("%d", reg[a0]);
            break;
        }
        case 4: // print_string
        {
            printf("%s", reg[a0]);
            break;
        }
        case 5: // read_int
        {
            cin >> reg[v0];
            break;
        }
        case 8: // read_string
        {
            uint32_t addr = reg[a0];
            size_t len = reg[a1];
            if (len < 1)
                break;
            else if (len == 1)
            {
                char ch = '\0';
                string s = bitset<byte_size>(static_cast<unsigned long long>(ch)).to_string();
                byte_t byte;
                copy(s.begin(), s.end(), byte.data());
                store_byte_to_memory(byte, addr++);
            }
            else
            {
                for (size_t i = 0; i < len - 1; i++)
                {
                    char ch = getchar();
                    // If less than len-1, adds newline to end.
                    if (ch == '\0')
                        ch = '\n';
                    // convert char to string with size=8
                    string s = bitset<byte_size>(static_cast<unsigned long long>(ch)).to_string();
                    byte_t byte;
                    copy(s.begin(), s.end(), byte.data());
                    store_byte_to_memory(byte, addr++);
                }
                // pads with null byte
                char ch = '\0';
                string s = bitset<byte_size>(static_cast<unsigned long long>(ch)).to_string();
                byte_t byte;
                copy(s.begin(), s.end(), byte.data());
                store_byte_to_memory(byte, addr++);
            }
            break;
        }
        case 9: // sbrk
        {
            reg[v0] = dynamic_end_idx;
            dynamic_end_idx += reg[a0];
            break;
        }
        case 10: // exit
        {
            exit(0);
            break;
        }
        case 11: // print_char
        {
            printf("%c", reg[a0]);
            break;
        }
        case 12: // read_char
        {
            scanf("%c", &reg[v0]);
            break;
        }
        case 13: // open
        {
            const char *filename = to_string(reg[a0]).c_str();
            reg[v0] = open(filename, reg[a1], reg[a2]);
            break;
        }
        case 14: // read
        {
            reg[v0] = read(reg[a0], (void *)(reg[a1]), reg[a2]);
            break;
        }
        case 15: // write
        {
            reg[v0] = write(reg[a0], (const void *)reg[a1], reg[a2]);
            break;
        }
        case 16:
        {
            close(reg[a0]);
            break;
        }
        case 17:
        {
            exit(reg[a0]);
            break;
        default:
            break;
        }
        }
    }
};
int32_t &Simulator::get_regv(const string &reg_str)
{
    return reg[regcode_to_idx[reg_str]];
}
void Simulator::store_word_to_memory(const word_t &word, uint32_t addr)
{
    size_t idx = addr2idx(addr);
    size_t j = 0;
    for (size_t i = idx; i < idx + 4; i++)
    {
        for (size_t k = 0; k < byte_size; k++)
            memory[i][k] = word[j++];
    }
}
void Simulator::store_half_to_memory(const half_t &half, uint32_t addr)
{
    size_t idx = addr2idx(addr);
    size_t j = 0;
    for (size_t i = idx; i < idx + 2; i++)
    {
        for (size_t k = 0; k < byte_size; k++)
            memory[i][k] = half[j++];
    }
}
void Simulator::store_byte_to_memory(const byte_t &byte, uint32_t addr)
{
    size_t idx = addr2idx(addr);
    size_t j = 0;
    for (size_t k = 0; k < byte_size; k++)
        memory[idx][k] = byte[j++];
}
Simulator::word_t Simulator::get_word_from_memory(uint32_t addr)
{
    word_t word;
    size_t idx = addr2idx(addr);
    size_t j = 0;
    for (size_t i = idx; i < idx + 4; i++)
    {
        for (size_t k = 0; k < byte_size; k++)
            word[j++] = memory[i][k];
    }
    return word;
}
Simulator::byte_t Simulator::get_byte_from_memory(uint32_t addr)
{
    byte_t byte;
    size_t idx = addr2idx(addr);
    size_t j = 0;
    for (size_t k = 0; k < byte_size; k++)
        byte[j++] = memory[idx][k];
    return byte;
}
Simulator::half_t Simulator::get_half_from_memory(uint32_t addr)
{
    half_t half;
    size_t idx = addr2idx(addr);
    size_t j = 0;
    for (size_t i = idx; i < idx + 2; i++)
    {
        for (size_t k = 0; k < byte_size; k++)
            half[j++] = memory[i][k];
    }
    return half;
}
int32_t Simulator::get_wordval_from_memory(uint32_t addr)
{
    word_t word = get_word_from_memory(addr);
    string word_str;
    copy(word.begin(), word.end(), word_str.data());
    int32_t word_val = stoi(word_str, nullptr, 2);
}
int16_t Simulator::get_halfval_from_memory(uint32_t addr)
{
    half_t half = get_half_from_memory(addr);
    string half_str;
    copy(half.begin(), half.end(), half_str.data());
    int16_t half_val = stoi(half_str, nullptr, 2);
}
int8_t Simulator::get_byteval_from_memory(uint32_t addr)
{
    byte_t byte = get_byte_from_memory(addr);
    string byte_str;
    copy(byte.begin(), byte.end(), byte_str.data());
    int8_t byte_val = stoi(byte_str, nullptr, 2);
}
void Simulator::gen_opcode_to_func()
{
    /*
    Some I and J instructions 
    opcode -> instr_func
    Total 28
    */
    // 26 I instructions
    opcode_to_func.emplace("000100", instr_beq);
    opcode_to_func.emplace("000101", instr_bne);
    opcode_to_func.emplace("001000", instr_addi);
    opcode_to_func.emplace("001001", instr_addiu);
    opcode_to_func.emplace("001100", instr_andi);
    opcode_to_func.emplace("001101", instr_ori);
    opcode_to_func.emplace("001110", instr_xori);
    opcode_to_func.emplace("001010", instr_slti);
    opcode_to_func.emplace("001011", instr_sltiu);
    opcode_to_func.emplace("100011", instr_lw);
    opcode_to_func.emplace("101011", instr_sw);
    opcode_to_func.emplace("100000", instr_lb);
    opcode_to_func.emplace("100100", instr_lbu);
    opcode_to_func.emplace("100001", instr_lh);
    opcode_to_func.emplace("100101", instr_lhu);
    opcode_to_func.emplace("101000", instr_sb);
    opcode_to_func.emplace("101001", instr_sh);
    opcode_to_func.emplace("100010", instr_lwl);
    opcode_to_func.emplace("100110", instr_lwr);
    opcode_to_func.emplace("101010", instr_swl);
    opcode_to_func.emplace("101110", instr_swr);
    opcode_to_func.emplace("001111", instr_lui);
    opcode_to_func.emplace("110000", instr_ll);
    opcode_to_func.emplace("111000", instr_sc);
    opcode_to_func.emplace("000111", instr_bgtz);
    opcode_to_func.emplace("000110", instr_blez);
    // 2 J instructions
    opcode_to_func.emplace("000010", instr_j);
    opcode_to_func.emplace("000011", instr_jal);
}
void Simulator::gen_rt_to_func()
{
    /*
    special I instructions with opcode=000001
    rt -> instr_func
    Total 10
    */
    // 10 special I instructions with opcode=000001
    rt_to_func.emplace("00000", instr_bltz);
    rt_to_func.emplace("00001", instr_bgez);
    rt_to_func.emplace("10001", instr_bgezal);
    rt_to_func.emplace("10000", instr_bltzal);
    rt_to_func.emplace("01100", instr_teqi);
    rt_to_func.emplace("01110", instr_tnei);
    rt_to_func.emplace("01000", instr_tgei);
    rt_to_func.emplace("01001", instr_tgeiu);
    rt_to_func.emplace("01010", instr_tlti);
    rt_to_func.emplace("01011", instr_tltiu);
}
void Simulator::gen_opcode_funct_to_func()
{
    /*
    R instructions only
    opcode+funct -> instr_func
    Total 38
    */
    // 39 R instructions
    opcode_funct_to_func.emplace("000000100000", instr_add);
    opcode_funct_to_func.emplace("000000100001", instr_addu);
    opcode_funct_to_func.emplace("000000100010", instr_sub);
    opcode_funct_to_func.emplace("000000100011", instr_subu);
    opcode_funct_to_func.emplace("000000100100", instr_and);
    opcode_funct_to_func.emplace("000000100101", instr_or);
    opcode_funct_to_func.emplace("000000100110", instr_xor);
    opcode_funct_to_func.emplace("000000100111", instr_nor);
    opcode_funct_to_func.emplace("000000101010", instr_slt);
    opcode_funct_to_func.emplace("000000101011", instr_sltu);
    opcode_funct_to_func.emplace("000000000100", instr_sllv);
    opcode_funct_to_func.emplace("000000000110", instr_srlv);
    opcode_funct_to_func.emplace("000000000111", instr_srav);
    opcode_funct_to_func.emplace("000000011000", instr_mult);
    opcode_funct_to_func.emplace("000000011001", instr_multu);
    opcode_funct_to_func.emplace("000000011010", instr_div);
    opcode_funct_to_func.emplace("000000011011", instr_divu);
    opcode_funct_to_func.emplace("000000001001", instr_jalr);
    opcode_funct_to_func.emplace("000000000000", instr_sll);
    opcode_funct_to_func.emplace("000000000011", instr_sra);
    opcode_funct_to_func.emplace("000000000010", instr_srl);
    opcode_funct_to_func.emplace("000000010001", instr_mthi);
    opcode_funct_to_func.emplace("000000010011", instr_mtlo);
    opcode_funct_to_func.emplace("000000001000", instr_jr);
    opcode_funct_to_func.emplace("000000010000", instr_mfhi);
    opcode_funct_to_func.emplace("000000010010", instr_mflo);
    opcode_funct_to_func.emplace("000000110100", instr_teq);
    opcode_funct_to_func.emplace("000000110110", instr_tne);
    opcode_funct_to_func.emplace("000000110000", instr_tge);
    opcode_funct_to_func.emplace("000000110001", instr_tgeu);
    opcode_funct_to_func.emplace("000000110010", instr_tlt);
    opcode_funct_to_func.emplace("000000110011", instr_tltu);
    opcode_funct_to_func.emplace("000000100001", instr_clo);
    opcode_funct_to_func.emplace("000000100000", instr_clz);

    opcode_funct_to_func.emplace("011100000010", instr_mul);
    opcode_funct_to_func.emplace("011100000000", instr_madd);
    opcode_funct_to_func.emplace("011100000100", instr_msub);
    opcode_funct_to_func.emplace("011100000001", instr_maddu);
    opcode_funct_to_func.emplace("011100000101", instr_msubu);
}
void Simulator::exec_instr(const string &mc)
{
    const string syscall = "00000000000000000000000000001100";
    if (mc == syscall)
    {
        instr_syscall(mc);
        return;
    }
    string opcode = mc.substr(0, 6);
    const string R_opcode[2] = {"000000", "011100"};
    const string Ispecial_opcode = "000001";
    if (opcode == R_opcode[0] || opcode == R_opcode[1])
    {
        // R instructions
        string funct = mc.substr(mc.size() - 6, 6);
        auto it = opcode_funct_to_func.find(opcode + funct);
        if (it == opcode_funct_to_func.end())
        {
            cout << "function not found!" << endl;
        }
        (it->second)(mc);
    }
    else if (opcode == Ispecial_opcode)
    {
        // special I instructions with opcode=000001
        string rt = mc.substr(11, 5);
        auto it = rt_to_func.find(rt);
        if (it == rt_to_func.end())
        {
            cout << "function not found!" << endl;
        }
        (it->second)(mc);
    }
    else
    {
        // Some I and J instructions
        auto it = opcode_to_func.find(opcode);
        if (it == opcode_to_func.end())
        {
            cout << "function not found!" << endl;
        }
        (it->second)(mc);
    }
}
size_t Simulator::addr2idx(uint32_t vm)
{
    /*
    virual memory addr -> idx
    */
    return vm - base_vm;
}
size_t Simulator::idx2addr(size_t idx)
{
    /*
    idx -> virual memory addr
    */
    return idx + base_vm;
}
void Simulator::init_reg_value()
{
    size_t sp_idx = 29;
    reg[sp_idx] = 0xa00000;
}
void Simulator::store_text()
{
    size_t i;
    for (i = 0; i < output.size(); i++)
    {
        string s = output[i];
        if (s.find(".text") != string::npos)
        {
            ++i;
            break;
        }
    }
    for (i; i < output.size(); i++)
    {
        word_t word;
        for (size_t k = 0; k < word.size(); k++)
            word[k] = output[i][k];
        store_word_to_memory(word, idx2addr(text_end_idx));
        text_end_idx += 4;
    }
}
void Simulator::simulate()
{
    gen_regcode_to_idx();
    gen_opcode_to_func();
    init_reg_value();
    store_static_data();
    store_text();
    // start simulating
    pc = base_vm;
#ifdef DEBUG_SIM
    exec_instr("00100000100001000000000000000001");
#endif
    while (pc >= base_vm && pc < text_end_idx)
    {
        word_t word = get_word_from_memory(pc);
        string mc(begin(word), end(word));
        exec_instr(mc);
        pc += 4;
    }
}
void Simulator::gen_regcode_to_idx()
{
    for (size_t i = 0; i < reg_size; i++)
    {
        string s = bitset<5>(i).to_string();
        regcode_to_idx[s] = i;
    }
}
void Simulator::store_static_data()
{
    /*
    store .data
    output[0] == ".data"
    */
    for (size_t i = 1; i < output.size(); i++)
    {
        if (output[i].find(".text") == string::npos)
            break;
        word_t word;
        for (size_t k = 0; k < word.size(); k++)
            word[k] = output[i][k];
        store_word_to_memory(word, idx2addr(dynamic_st_idx));
        dynamic_st_idx += 4;
    }
}
int main(int argc, char *argv[])
{
    Assembler assembler;
    Simulator simulator(assembler.output);
    if (argc == 1)
    {
        assembler.scanner.scan(cin);
        assembler.parser.parse();
        simulator.simulate();
    }
    else if (argc == 2)
    {
        ifstream asmin(argv[1]);
        if (!asmin.is_open())
        {
            cout << argv[1] << "can not open" << endl;
            return 0;
        }
        assembler.scanner.scan(asmin);
        assembler.parser.parse();
        simulator.simulate();
    }
    else if (argc == 3)
    {
        ifstream asmin(argv[1]);
        ofstream fileout(argv[2]);
        if (!asmin.is_open())
        {
            cout << argv[1] << "can not open" << endl;
            return 0;
        }
        if (!fileout.is_open())
        {
            cout << argv[2] << "can not open" << endl;
            return 0;
        }
        assembler.scanner.scan(asmin);
        assembler.parser.parse();
        simulator.simulate();
    }
    else if (argc == 4)
    {
        ifstream asmin(argv[1]);
        ifstream filein(argv[2]);
        ofstream fileout(argv[3]);
        if (!asmin.is_open())
        {
            cout << argv[1] << "can not open" << endl;
            return 0;
        }
        if (!filein.is_open())
        {
            cout << argv[2] << "can not open" << endl;
            return 0;
        }
        if (!fileout.is_open())
        {
            cout << argv[3] << "can not open" << endl;
            return 0;
        }
        assembler.scanner.scan(asmin);
        assembler.parser.parse();
        simulator.simulate();
    }
    return 0;
}