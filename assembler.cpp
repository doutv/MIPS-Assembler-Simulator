#include <iostream>
#include <algorithm>
#include <bitset>
#include <string>
#include <cstring>
#include <vector>
#include <unordered_map>
using namespace std;

class Scanner
{
public:
    vector<string> file;
    vector<string> data_seg;
    vector<string> text_seg;
    void remove_comments();
    void split_data_and_text();
    Scanner(vector<string> &in_file)
    {
        file = in_file;
        remove_comments();
        split_data_and_text();
    }
    const vector<string> &get_data_seg()
    {
        return data_seg;
    }
    const vector<string> &get_text_seg()
    {
        return text_seg;
    }
    void remove_comments()
    {
        for (string &s : file)
        {
            if (s.empty())
            {
                continue;
            }
            for (size_t i = 0; i < s.size(); i++)
            {
                if (s[i] == '#')
                {
                    s.erase(i, s.size());
                }
            }
        }
    }
    /*
    split data and text part
    remove empty lines
    */
    void split_data_and_text()
    {
        size_t i;
        for (i = 0; i < file.size(); i++)
        {
            string s = file[i];
            if (s.empty())
                continue;
            if (s.find(".text") != string::npos)
            {
                break;
            }
            data_seg.push_back(s);
        }
        for (i = i + 1; i < file.size(); i++)
        {
            string s = file[i];
            if (s.empty())
                continue;
            text_seg.push_back(s);
        }
    }
};

class Parser
{
    enum optype
    {
        O_type,
        R_type,
        I_type,
        J_type,
        P_type
    };
    const vector<string> data_seg;
    const vector<string> text_seg;
    unordered_map<string, uint32_t> label_to_addr;
    uint32_t pc;
    uint32_t get_R_instruction(string op, string rs, string rt, string rd, string shamt, string funct);
    uint32_t get_I_instruction(string op, string rs, string rt, string immediate);
    uint32_t get_J_instruction(string op, string address);
    string get_next_token();
    int get_op_type(const string &op);
    void parse();
    Parser(const vector<string> &in_data_seg, const vector<string> &in_text_seg)
    {
        pc = 0x400000;
        data_seg = in_data_seg;
        text_seg = in_text_seg;
        parse();
    }

    string get_register_code(const string &r)
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
    int get_op_type(const string &op)
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
                 op == "bgez" || op == "bgtz")
            return I_type;
        else if (op == "j" || op == "jal")
            return J_type;
        else
            return P_type;
    }
    string sign_extent(const string &s, const size_t target)
    {
        string res;
        int32_t num = stoi(s);
        bool is_positive;
        num ? is_positive = 1 : 0;
        while (num)
        {
            res.push_back((num & 1) + '0');
            num >>= 1;
        }
        while (res.size() < target)
        {
            if (is_positive)
                res.push_back('0');
            else
                res.push_back('1');
        }
        if (res.size() > target)
        {
            // overflow
            res = res.substr(0, target);
        }
        reverse(res.begin(), res.end());
        return res;
    }
    /*
    convert string in decimal to string in binary and add zero to its head
    */
    string zero_extent(const string &s, const size_t target)
    {
        string res;
        int32_t num = stoi(s);
        while (num)
        {
            res.push_back((num & 1) + '0');
            num >>= 1;
        }
        while (res.size() < target)
        {
            res.push_back('0');
        }
        reverse(res.begin(), res.end());
        return res;
    }
    void parse()
    {
        for (string &s : text_seg)
        {
            size_t i = s.find(':') == string::npos ? 0 : s.find(':') + 1;
            if (i)
            {
                // locate label
                size_t st = 0;
                while (st < s.size() && s[st] == ' ')
                    ++st;
                string label = s.substr(st, i - st);
                label_to_addr[label] = pc;
            }
            for (i; i < s.size(); i++)
            {
                if (s[i] == ' ')
                    continue;
                string op = s.substr(i, s.find(' ', i) - i);
                switch (get_op_type(op))
                {

                case R_type:
                    get_R_instruction(op);
                    break;
                case I_type:
                    get_I_instruction(op);
                    break;
                case J_type:
                    get_J_instruction(op);
                    break;
                default:
                    break;
                }
            }
            pc += 4;
        }
    }
    uint32_t get_R_instruction(const string &op)
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
        string opCode, dReg, sReg, tReg, shAmt, func;
        string temp;
        opCode = "000000";
        // op rd rs rt
        if (op == "add" || op == "addu" || op == "sub" || op == "subu" ||
            op == "and" || op == "or" || op == "xor" || op == "nor" ||
            op == "slt" || op == "sltu")
        {
            temp = get_next_token();
            dReg = get_register_code(temp);

            temp = get_next_token();
            sReg = get_register_code(temp);

            temp = get_next_token();
            tReg = get_register_code(temp);

            shAmt = "00000";

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
            dReg = get_register_code(temp);

            temp = get_next_token();
            tReg = get_register_code(temp);

            temp = get_next_token();
            sReg = get_register_code(temp);

            shAmt = "00000";

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
            sReg = get_register_code(temp);

            temp = get_next_token();
            tReg = get_register_code(temp);

            dReg = "00000";

            shAmt = "00000";

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
            sReg = get_register_code(temp);

            temp = get_next_token();
            dReg = get_register_code(temp);

            shAmt = "00000";
            tReg = "00000";

            func = "001001";
        }

        // op rd rt shamt
        else if (op == "sll" || op == "srl" || op == "sra")
        {
            sReg = "00000";
            temp = get_next_token();
            dReg = get_register_code(temp);
            temp = get_next_token();
            tReg = get_register_code(temp);

            shAmt = get_next_token();
            shAmt = zero_extent(shAmt, 5);

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
            sReg = get_register_code(temp);
            tReg = "00000";
            dReg = "00000";
            shAmt = "00000";

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
            dReg = get_register_code(temp);
            sReg = "00000";
            tReg = "00000";
            shAmt = "00000";

            if (op == "mfhi")
                func = "010000";
            else if (op == "mflo")
                func = "010010";
            else
                ;
        }
        cout << opCode + dReg + sReg + tReg + shAmt + func << endl;
    }

    uint32_t get_I_instruction(const string &op)
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
        string opCode, sReg, tReg, imme;
        string temp;
        uint32_t addr;
        // op rs rt imme
        if (op == "beq" || op == "bne")
        {
            temp = get_next_token();
            sReg = get_register_code(temp);

            temp = get_next_token();
            tReg = get_register_code(temp);

            temp = get_next_token();
            addr = label_to_addr[temp];
            addr = (addr - (pc + 4)) / 4;
            temp = to_string(addr);
            imme = sign_extent(temp, 16);

            if (op == "beq")
                opCode = "000100";
            else if (op == "bne")
                opCode = "000101";
            else
                ;
        }

        // op rt ts imme
        else if (op == "addi" || op == "addiu" || op == "andi" || op == "ori" ||
                 op == "xori" || op == "slti" || op == "slti" || op == "sltiu")
        {
            temp = get_next_token();
            tReg = get_register_code(temp);

            temp = get_next_token();
            sReg = get_register_code(temp);

            temp = get_next_token();

            if (op == "addi" || op == "slti" || op == "sltiu" || op == "addiu")
                imme = sign_extent(temp, 16);
            else
                imme = zero_extent(temp, 16);

            if (op == "addi")
                opCode = "001000";
            else if (op == "addiu")
                opCode = "001001";
            else if (op == "andi")
                opCode = "001100";
            else if (op == "ori")
                opCode = "001101";
            else if (op == "xori")
                opCode = "001110";
            else if (op == "slti")
                opCode = "001010";
            else if (op == "sltiu")
                opCode = "001011";
            else
                ;
        }

        // op rt imme(rs)
        else if (op == "lw" || op == "sw" || op == "lb" || op == "lbu" || op == "lh" ||
                 op == "lhu" || op == "sb" || op == "sb" || op == "sh")
        {
            temp = get_next_token();
            tReg = get_register_code(temp);

            temp = get_next_token();
            sr_imme = temp.split(QRegExp("[()]"));

            imme = sign_extent(sr_imme.at(0), 16);
            sReg = get_register_code(sr_imme.at(1));

            if (op == "lw")
                opCode = "100011";
            else if (op == "sw")
                opCode = "101011";
            else if (op == "lb")
                opCode = "100000";
            else if (op == "lbu")
                opCode = "100100";
            else if (op == "lh")
                opCode = "100001";
            else if (op == "lhu")
                opCode = "100101";
            else if (op == "sb")
                opCode = "101000";
            else if (op == "sh")
                opCode = "101001";
            else
                ;
        }

        // op rt imme
        else if (op == "lui")
        {
            temp = get_next_token();
            tReg = get_register_code(temp);

            sReg = "00000";

            temp = get_next_token();
            imme = sign_extent(temp, 16);

            opCode = "001111";
        }

        // op rs imme
        else if (op == "blez" || op == "bltz" || op == "bgtz" || op == "bgez")
        {
            temp = get_next_token();
            sReg = get_register_code(temp);

            temp = get_next_token();
            addr = label_to_addr[temp];
            addr = (addr - (pc + 4)) / 4;
            temp = to_string(addr);
            imme = sign_extent(temp, 16);

            if (op == "bgez")
            {
                opCode = "000001";
                tReg = "00001";
            }
            else if (op == "bgtz")
            {
                opCode = "000111";
                tReg = "00000";
            }
            else if (op == "blez")
            {
                opCode = "000110";
                tReg = "00000";
            }
            else if (op == "bltz")
            {
                opCode = "000001";
                tReg = "00000";
            }

            else
                ;
        }
        cout << opCode + sReg + tReg + imme << endl;
    }

    uint32_t get_J_instruction(const string &op)
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
        string opCode, target;
        uint32_t addr;
        if (op == "j")
            opCode = "000010";
        else if (op == "jal")
            opCode = "000011";
        else
            ;

        target = get_next_token();
        addr = label_to_addr[target];
        addr >>= 2;
        target = to_string(addr);
        target = sign_extent(target, 26);

        cout << opCode + target << endl;
    }
};

int main()
{
    vector<string> ori_file;
    string s;
    while (getline(cin, s))
    {
        ori_file.push_back(s);
    }
    Scanner scanner(ori_file);
    Parser parser(scanner.get_data_seg(), scanner.get_text_seg());

    return 0;
}