// #define DEBUG_ASS
#define DEBUG_DATA

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
            label_to_addr[label] = label_pc;
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
    if (op == "eret" || op == "syscall" || op == "break" || op == "nop")
        return O_type;
    else if (op == "add" || op == "addu" || op == "sub" || op == "subu" ||
             op == "and" || op == "or" || op == "xor" || op == "nor" ||
             op == "slt" || op == "sltu" || op == "sll" || op == "srl" ||
             op == "sra" || op == "sllv" || op == "srlv" || op == "srav" ||
             op == "jr" || op == "div" || op == "divu" || op == "mult" ||
             op == "multu" || op == "jalr" || op == "mtlo" || op == "mthi" ||
             op == "mfhi" || op == "mflo")
        return R_type;

    else if (op == "addi" || op == "addiu" || op == "andi" || op == "ori" ||
             op == "xori" || op == "lui" || op == "lw" || op == "sw" ||
             op == "beq" || op == "bne" || op == "slti" || op == "sltiu" ||
             op == "lb" || op == "lbu" || op == "lh" || op == "lhu" ||
             op == "sb" || op == "sh" || op == "blez" || op == "bltz" ||
             op == "bgez" || op == "bgtz" || op == "lwl" || op == "lwr" ||
             op == "swl" || op == "swr")
        return I_type;
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
    if (op == "nop")
        machine_code = "00000000000000000000000000000000";
    else if (op == "eret")
        machine_code = "01000010000000000000000000011000";
    else if (op == "syscall")
        machine_code = "00000000000000000000000000001100";
    else if (op == "break")
    {
        string temp;
        temp = get_next_token();
        temp = zero_extent(temp, 20);
        machine_code = "000000" + temp + "001101";
    }
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
    string opcode, rd, rs, rt, shamt, func;
    string temp;
    opcode = "000000";
    // op rd rs rt
    if (op == "add" || op == "addu" || op == "sub" || op == "subu" ||
        op == "and" || op == "or" || op == "xor" || op == "nor" ||
        op == "slt" || op == "sltu")
    {
        temp = get_next_token();
        rd = get_register_code(temp);

        temp = get_next_token();
        rs = get_register_code(temp);

        temp = get_next_token();
        rt = get_register_code(temp);

        shamt = "00000";

        if (op == "add")
            func = "100000";
        else if (op == "addu")
            func = "100001";
        else if (op == "sub")
            func = "100010";
        else if (op == "subu")
            func = "100011";
        else if (op == "and")
            func = "100100";
        else if (op == "or")
            func = "100101";
        else if (op == "xor")
            func = "100110";
        else if (op == "nor")
            func = "100111";
        else if (op == "slt")
            func = "101010";
        else if (op == "sltu")
            func = "101011";
        else
            ;
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
            func = "000100";
        else if (op == "srlv")
            func = "000110";
        else if (op == "srav")
            func = "000111";
        else
            ;
    }

    // op rs rt
    else if (op == "mult" || op == "multu" || op == "div" || op == "divu")
    {
        temp = get_next_token();
        rs = get_register_code(temp);

        temp = get_next_token();
        rt = get_register_code(temp);

        rd = "00000";

        shamt = "00000";

        if (op == "mult")
            func = "011000";
        else if (op == "multu")
            func = "011001";
        else if (op == "div")
            func = "011010";
        else if (op == "divu")
            func = "011011";
        else
            ;
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

        func = "001001";
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
            func = "000000";
        else if (op == "sra")
            func = "000011";
        else if (op == "srl")
            func = "000010";
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
            func = "010001";
        else if (op == "mtlo")
            func = "010011";
        else if (op == "jr")
            func = "001000";
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
            func = "010000";
        else if (op == "mflo")
            func = "010010";
        else
            ;
    }
    string machine_code = opcode + rs + rt + rd + shamt + func;
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
        addr = label_to_addr[temp];
        addr = (addr - (pc + 4)) / 4;
        temp = to_string(addr);
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
             op == "lwr" || op == "swl" || op == "swr")
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
    else if (op == "blez" || op == "bltz" || op == "bgtz" || op == "bgez")
    {
        temp = get_next_token();
        rs = get_register_code(temp);

        temp = get_next_token();
        addr = label_to_addr[temp];
        addr = (addr - (pc + 4)) / 4; // compute offset
        imme = zero_extent(to_string(addr), 16);

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

        else
            ;
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
    addr = label_to_addr[target];
    addr >>= 2;
    target = to_string(addr);
    target = zero_extent(target, 26);

    string machine_code = opcode + target;
    return machine_code;
}

class Simulator
{
    Simulator();
};

int main(int argc, char *argv[])
{
    Assembler assembler;
    if (argc == 1)
    {
        assembler.scanner.scan(cin);
        assembler.parser.parse();
        assembler.parser.print_machine_code(cout);
    }
    else if (argc == 2)
    {
        ifstream filein(argv[1]);
        if (!filein.is_open())
        {
            cout << argv[1] << "can not open" << endl;
            return 0;
        }
        assembler.scanner.scan(filein);
        assembler.parser.parse();
        assembler.parser.print_machine_code(cout);
    }
    else if (argc == 3)
    {
        ifstream filein(argv[1]);
        ofstream fileout(argv[2]);
        if (!filein.is_open())
        {
            cout << argv[1] << "can not open" << endl;
            return 0;
        }
        if (!fileout.is_open())
        {
            cout << argv[2] << "can not open" << endl;
            return 0;
        }
        assembler.scanner.scan(filein);
        assembler.parser.parse();
        assembler.parser.print_machine_code(fileout);
    }

    return 0;
}