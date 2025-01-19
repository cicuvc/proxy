#include <iostream>
#include "proxy.hpp"

interface_def(Drawable){
    support_copy(pro::constraint_level::nontrivial);
    fn_def(Area, double() const, double(double) const);
    fn_def(Info, void() const);
}interface_end(Drawable);

template<typename ... Args>
std::string string_format( const std::string& format, Args ... args )
{
    int size_s = std::snprintf( nullptr, 0, format.c_str(), args ... ) + 1;
    if( size_s <= 0 ){ throw std::runtime_error( "Error during formatting." ); }
    auto size = static_cast<size_t>( size_s );
    std::unique_ptr<char[]> buf( new char[ size ] );
    std::snprintf( buf.get(), size, format.c_str(), args ... );
    return std::string( buf.get(), buf.get() + size - 1 );
}

struct Rectangle{
    Rectangle(double width_, double height_): width(width_), height(height_){}

    double Area() const {return width * height;}
    double Area(double bias) const {return width * height + bias;}

    void Info() const{
        auto s = string_format("Rectangle[w = %.2f, h = %.2f]", width, height);
        std::cout << s << std::endl;
    }
    private:
    double width;
    double height;
};

int main(){
    auto proxy = pro::make_proxy<Drawable>(Rectangle(1.0, 2.0));

    std::cout << string_format("Area = %.2f", proxy->Area()) << std::endl;
    std::cout << string_format("Area + 1.0 = %.2f", proxy->Area(1.0)) << std::endl;

    proxy->Info();
}