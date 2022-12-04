#include <iostream>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <map>
#include <paint/paint.h>

static int KERNEL_DIM = 16;

#define BLACK Color(0, 0, 0)
#define WHITE Color(255, 255, 255)

struct Color {
    unsigned char r, g, b;
    Color(unsigned char r = 0, unsigned char g = 0, unsigned char b = 0)
            : r(r), g(g), b(b) {}
    operator int() const {
        return (r << 16) | (g << 8) | b;
    }
};

static_assert(sizeof(Color) == 3);

template<class T>
static T clamp(T v, T min, T max) {
    return v < min ? min : v > max ? max : v;
}

template<class T>
struct Matrix {
    int width;
    int height;
    std::vector<T> data;
public:
    int offset(int x, int y) const { return y * width + x; }
    T get(int x, int y) const { return data[offset(x, y)]; }
    void set(int x, int y, T v) { data[offset(x, y)] = v; }
    Matrix(int width, int height, std::vector<Color> &&data)
            : width(width), height(height), data(data) {}
    Matrix() : width(0), height(0) {}
};

class PPM {
    std::fstream file;
    int width, height;
    std::streampos datapos;
public:
    int getheight() const { return height; }
    int getwidth() const { return width; }
    PPM(const std::string &path)
            : file(path, std::ios::binary | std::ios::in | std::ios::out) {
        std::string magic;
        int max;
        file >> magic;
        file >> width >> height;
        file >> max;
        file.get(); // skip last whitespace
        datapos = file.tellg();
    }
    static void create(const std::string &path, int dim) {
        std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
        ofs << "P6" << std::endl;
        ofs << dim << " " << dim << std::endl;
        ofs << "255" << std::endl;
        for (int i = 0; i < dim * dim; i++) {
            int marked = std::rand() & 1;
            Color c = marked ? BLACK : WHITE;
            ofs.write((char*)&c, sizeof(c));
        }
    }
    Matrix<Color> read(int x, int y, int width, int height) {
        if (x < 0 || x >= this->width || y < 0 || y >= this->height)
            return Matrix<Color>();
        width = clamp(width, 0, this->width - x);
        height = clamp(height, 0, this->height - y);
        std::vector<Color> data(width * height);
        for (int row = 0; row < height; row++) {
            auto filepos = datapos;
            filepos += (y + row) * this->width * sizeof(Color);
            filepos += x * sizeof(Color);
            file.seekg(filepos);
            file.read((char*)&data[row * width], width * sizeof(Color));
        }
        return Matrix<Color>(width, height, std::move(data));
    }
    void write(int x, int y, const Matrix<Color> &matrix) {
        if (x < 0 || x >= this->width || y < 0 || y >= this->height)
            return;
        int width = clamp(matrix.width, 0, this->width - x);
        int height = clamp(matrix.height, 0, this->height - y);
        for (int row = 0; row < height; row++) {
            auto filepos = datapos;
            filepos += (y + row) * this->width * sizeof(Color);
            filepos += x * sizeof(Color);
            file.seekp(filepos);
            file.write((char*)&matrix.data[matrix.offset(0, row)],
                    width * sizeof(Color));
        }
    }
};

class Painter {
    std::map<Color, Color> colors;
    int ncolors = 0;
    Color _randcolor() const {
        Color c;
        c.r = 50 + rand() % 150;
        c.g = 50 + rand() % 150;
        c.b = 50 + rand() % 150;
        return c;
    }
    Color newcolor() {
        ncolors++;
        return _randcolor();
    }
    bool isblack(Color c) const { return c == BLACK; }
    bool iswhite(Color c) const { return c == WHITE; }
    bool ismarked(Color c) const { return !iswhite(c); }
    void colorleftcol(Matrix<Color> &m) {
        Color prev = m.get(0, 0);
        if (isblack(prev)) prev = newcolor();
        for (int y = 0; y < m.height; y++) {
            Color c = m.get(0, y);
            if (ismarked(c))
                c = ismarked(prev) ? prev : newcolor();
            prev = c;
            m.set(0, y, c);
        }
    }
    void colortoprow(Matrix<Color> &m) {
        Color prev = m.get(0, 0);
        if (isblack(prev)) prev = newcolor();
        for (int x = 0; x < m.width; x++) {
            Color c = m.get(x, 0);
            if (ismarked(c))
                c = ismarked(prev) ? prev : newcolor();
            prev = c;
            m.set(x, 0, c);
        }
    }
    Color resolve(Color c) {
        auto it = colors.find(c);
        if (it == colors.end()) return c;
        return resolve(it->second);
    }
    void mapcolors(Color from, Color to) {
        Color fromrc = resolve(from);
        Color torc = resolve(to);
        if (fromrc != torc) {
            colors[fromrc] = torc;
            ncolors--;
        }
    }
    void color(Matrix<Color> &m) {
        for (int y = 1; y < m.height; y++) {
            for (int x = 1; x < m.width; x++) {
                Color c = m.get(x, y);
                if (!ismarked(c)) continue;
                Color tc = m.get(x, y - 1);
                Color lc = m.get(x - 1, y);
                if (ismarked(tc) && ismarked(lc))
                    mapcolors(lc, tc);
                if (ismarked(tc))  c = tc;
                else if (ismarked(lc))  c = lc;
                else c = newcolor();
                m.set(x, y, c);
            }
        }
    }
    void resolve(Matrix<Color> &m) {
        for (int y = 0; y < m.height; y++)
            for (int x = 0; x < m.width; x++)
                m.set(x, y, resolve(m.get(x, y)));
    }
    void resolve(PPM &ppm) {
        int x = 0;
        int y = 0;
        while (y < ppm.getheight()) {
            while (x < ppm.getwidth()) {
                auto m = ppm.read(x, y, KERNEL_DIM, KERNEL_DIM);
                resolve(m);
                ppm.write(x, y, m);
                x += KERNEL_DIM;
            }
            x = 0;
            y += KERNEL_DIM;
        }
    }
public:
    void paint(PPM &ppm) {
        int x = 0;
        int y = 0;
        while (y < ppm.getheight()) {
            while (x < ppm.getwidth()) {
                auto m = ppm.read(x, y, KERNEL_DIM, KERNEL_DIM);
                if (x == 0) colorleftcol(m);
                if (y == 0) colortoprow(m);
                color(m);
                ppm.write(x, y, m);
                x += KERNEL_DIM - 1;
            }
            x = 0;
            y += KERNEL_DIM - 1;
        }
        resolve(ppm);
        std::cout << "shapes: " << ncolors << std::endl;
        colors.clear();
        ncolors = 0;
    }
};

static void copyfile(const std::string &from, const std::string &to) {
    std::ifstream ifs(from, std::ios::binary);
    std::ofstream ofs(to, std::ios::binary);
    ofs << ifs.rdbuf();
}

static const std::vector<std::string> OPTS = {
    {"-g dim out.ppm:generate a random ppm image of square size dim"},
    {"-k num:set kernel size"}
};

static void usage() {
    printf("usage: paint [options] input.ppm output.ppm\n");
    printf("%4s%s\n", "", "paints a black and white .ppm image");
    for (auto &opt : OPTS) {
        auto flag = opt.substr(0, opt.find(':'));
        auto desc = opt.substr(opt.find(':') + 1);
        printf("%4s%-20s%s\n", "", flag.c_str(), desc.c_str());
    }
}

int main(int argc, char **argv) {
    std::srand(std::clock());
    bool gen = false;
    int dim = 0;
    int kdim = 0;
    std::string inpath;
    std::string outpath;
    for (int i = 1; i < argc; i++) {
        if (argv[i] == std::string("-k")) {
            i++;
            kdim = std::stoi(argv[i]);
            KERNEL_DIM = kdim < 2 ? 2 : kdim;
        }
        else if (argv[i] == std::string("-g")) {
            gen = true;
            i++;
            dim = std::stoi(argv[i]);
        } else if (inpath.empty()) inpath = argv[i];
        else outpath = argv[i];
    }
    if (gen) {
        if (dim <= 0 || inpath.empty()) { usage(); return 1; }
        PPM::create(inpath, dim);
    }
    else {
        if (inpath.empty() || outpath.empty()) { usage(); return 1; }
        std::cout << "kernel size: " << KERNEL_DIM << std::endl;
        copyfile(inpath, outpath);
        PPM out(outpath);
        Painter painter;
        painter.paint(out);
    }
    return 0;
}
