#include <algorithm>
#include <cctype>
#include <fstream>
#include <functional>
#include <sstream>
#include <string>
#include <unordered_map>

class error_code;
class base_t;
class base_number;

typedef std::unordered_map<std::string, std::function<base_number(base_number, base_number)>> binary_f_t;
typedef std::unordered_map<std::string, std::function<base_number(base_number, int)>> binary_int_f_t;
typedef std::unordered_map<std::string, std::function<bool(base_number, base_number)>> boolean_f_t;

binary_f_t binary_f_map;
binary_int_f_t binary_int_f_map;
boolean_f_t boolean_f_map;

std::string process_line(const std::string& line);
void process(std::istream& is, std::ostream& os);
void populate_map(binary_f_t& x);
void populate_map(binary_int_f_t& x);
void populate_map(boolean_f_t& x);

class base_t
{
    enum limits
    {
        min = 2,
        max = 35
    };

    public:
    base_t() : value_(10) { }
    base_t(int new_value) { value(new_value); }

    int value() const { return value_; }

    void value(int new_value) {
        if (new_value < limits::min || new_value > limits::max)
        {
            std::stringstream ss;
            ss << "argument must be in the range[" << limits::min << ", " << limits::max << "]";
            throw std::invalid_argument(ss.str());
        }
        else
        {
            value_ = new_value;
        }
    }

    friend std::istream& operator>> (std::istream& is, base_t& rhs)
    {
        int temp;
        is >> temp;
        rhs.value(temp);
        return is;
    };

    private:
    int value_;
};

class base_number
{
    public:
    base_number() : base_(), number_() {}

    base_number(int new_base, const std::string& new_number)
    {
        base(new_base);
        number(new_number);
    }

    // TODO: bug -> number may contain characters invalid in base

    int base() const { return base_.value(); }
    void base(int new_base) { base_.value(new_base); }

    const std::string& number() const { return number_; }
    void number(const std::string& new_number) { number_ = new_number; }

    static int from_symbol(char c)
    {
        int rv;
        c = std::tolower(c);
        if (c >= '0' && c <= '9')
            rv = c - '0';
        else if (c >= 'a' && c <= 'z')
            rv = c - 'a' + 10;
        else
            throw std::invalid_argument("argument must in in the range [0-9a-zA-Z]");
        return rv;
    }

    static char to_symbol(int n)
    {
        char rv;
        if (n >= 0 && n <= 9)
            rv = n + '0';
        else if (n < 36)
            rv = n - 10 + 'A';
        else
            throw std::invalid_argument("argument must in in the range [0-35]");
        return rv;
    }

    base_number to_base(int new_base) const { return from_raw(raw(), new_base); };

    base_number from_raw(int raw_value, int new_base) const
    {
        std::string new_number;
        new_number.reserve(4);

        while (raw_value > 0)
        {
            new_number += to_symbol(raw_value % new_base);
            raw_value /= new_base;
        }

        std::reverse(new_number.begin(), new_number.end());

        return base_number(new_base, new_number);
    };

    int raw() const
    {
        int rv = 0;

        for (int i = 0; i < number_.size(); i++)
            rv = rv * base() + from_symbol(number_[i]);

        return rv;
    };

    bool operator== (const base_number& rhs) const { return raw() == rhs.raw(); }
    bool operator< (const base_number& rhs) const { return raw() < rhs.raw(); }

    bool operator!= (const base_number& rhs) const { return !(*this == rhs); }
    bool operator>= (const base_number& rhs) const { return !(*this < rhs); }
    bool operator<= (const base_number& rhs) const { return (*this < rhs) || (*this == rhs); }
    bool operator> (const base_number& rhs) const { return !(*this < rhs) && !(*this == rhs); }

    base_number& operator+= (const base_number& rhs)
    {
        *this = from_raw(raw() + rhs.raw(), base());
        return *this;
    }

    friend base_number operator+ (base_number lhs, const base_number& rhs)
    {
        lhs += rhs;
        return lhs;
    }

    base_number& operator*= (const base_number& rhs)
    {
        *this = from_raw(raw() * rhs.raw(), base());
        return *this;
    }

    friend base_number operator* (base_number lhs, const base_number& rhs)
    {
        lhs *= rhs;
        return lhs;
    }

    // TODO: Include support for subtraction and division

    friend std::istream& operator>> (std::istream& is, base_number& rhs)
    {
        is >> rhs.number_ >> rhs.base_;
        return is;
    };

    friend std::ostream& operator<< (std::ostream& os, const base_number& rhs)
    {
        os << rhs.number() << ' ' << rhs.base();
        return os;
    }

    private:
    base_t base_;
    std::string number_;
};

int main(int argc, char* argv[])
{
    populate_map(binary_f_map);
    populate_map(binary_int_f_map);
    populate_map(boolean_f_map);

    std::fstream in_file("in_test.txt", std::fstream::in);
    std::ofstream out_file("out_test.txt", std::fstream::out | std::fstream::trunc);

    process(in_file, out_file);

    in_file.close();
    out_file.close();

    getchar();

    return 0;
}

std::string process_line(const std::string& line)
{
    constexpr auto error_message = "ERROR";
    std::stringstream ss;
    std::ostringstream os;
    ss.str(line);

    base_number lhs;
    if ((ss >> lhs).fail()) return error_message;

    std::string op;
    if ((ss >> op).fail()) return error_message;

    // binary function -> base_number (base_number, base_number)
    if (binary_f_map.find(op) != binary_f_map.end())
    {
        std::string dummy;
        base_number rhs;

        if ((ss >> rhs).fail()) return error_message;

        auto result = binary_f_map[op](lhs, rhs);
        ss >> dummy;

        int new_base;
        if ((ss >> new_base).fail()) return error_message;

        os << result.to_base(new_base).number();
    }

    // binary function with int -> base_number (base_number, int)
    else if (binary_int_f_map.find(op) != binary_int_f_map.end())
    {
        int rhs;
        if ((ss >> rhs).fail()) return error_message;

        os << binary_int_f_map[op](lhs, rhs).number();
    }

    // boolean function -> bool (base_number, base_number)
    else if (boolean_f_map.find(op) != boolean_f_map.end())
    {
        base_number rhs;
        if ((ss >> rhs).fail()) return error_message;

        os << std::boolalpha << boolean_f_map[op](lhs, rhs);
    }

    return os.str();
}

void process(std::istream& is, std::ostream& os)
{
    std::string result;
    do
    {
        std::string line;
        is >> std::ws;
        getline(is, line);
        if (line == "!") break;
        result = process_line(line);
        if (result.empty()) break;
        os << result << '\n';
    } while (1);
}

void populate_map(binary_f_t& x)
{
    x["+"] = std::plus<base_number>();
    x["*"] = std::multiplies<base_number>();
}

void populate_map(binary_int_f_t& x)
{
    x["%"] = [](const base_number& lhs, int rhs){ return lhs.to_base(rhs); };
}

void populate_map(boolean_f_t& x)
{
    x["<"] = std::less<base_number>();
    x[">"] = std::greater<base_number>();
    x["="] = std::equal_to<base_number>();
}