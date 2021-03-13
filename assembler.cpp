#include <iostream>
#include <string>
#include <cstring>
#include <vector>
using namespace std;

// vector<string> ori_file;
// vector<string> data_seg;
// vector<string> text_seg;

class Scanner()
{
public:
    vector<string> file;
    vector<string> data_seg;
    vector<string> text_seg;
    void remove_comments();
    void split_data_and_text();
    Scanner(vector<string> & file)
    {
        this.file = file;
        remove_comments();
        split_data_and_text();
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
    void split_data_and_text()
    {
        // pass
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
}

class Parser()
{
}

int32_t get_R_instruction(int32_t op, int32_t rs, int32_t rt, int32_t rd, int32_t shamt, int32_t funct)
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
}

int32_t get_I_instruction(int32_t op, int32_t rs, int32_t rt, int32_t immediate)
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
}

int32_t get_J_instruction(int32_t op, int32_t address)
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
}

void parse()
{
    for (string &s : ori_file)
    {
        for (size_t i = 0; i < s.size(); i++)
        {
            if (s[i] == ' ')
                continue;
        }
    }
}
int main()
{
    string s;
    while (getline(cin, s))
    {
        ori_file.push_back(s);
    }
    remove_comments();

    return 0;
}