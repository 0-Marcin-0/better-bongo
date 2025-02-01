#include "header.hpp"

#include <X11/Xatom.h>
#include <X11/extensions/shape.h>

bool setShape(Window wnd, const sf::Image& image)
{
    Display* display = XOpenDisplay(NULL);

    // Setting the window shape requires the XShape extension
    int event_base;
    int error_base;
    if (!XShapeQueryExtension(display, &event_base, &error_base))
    {
        XCloseDisplay(display);
        return false;
    }

    const sf::Uint8* pixelData = image.getPixelsPtr();

    // Create a black and white pixmap that has the size of the window
    Pixmap pixmap = XCreatePixmap(display, wnd, image.getSize().x, image.getSize().y, 1);
    GC gc = XCreateGC(display, pixmap, 0, NULL);

    // Make the entire pixmap white
    XSetForeground(display, gc, 1);
    XFillRectangle(display, pixmap, gc, 0, 0, image.getSize().x, image.getSize().y);

    // Loop over the pixels in the image and change the color of the pixmap to black
    // for each pixel where the alpha component equals 0.
    // As an optimization, we will combine adjacent transparent pixels on the same row and
    // draw them together, instead of calling "XFillRectangle(display, pixmap, gc, x, y, 1, 1)"
    // for each transparent pixel individually.
    XSetForeground(display, gc, 0);
    bool transparentPixelFound = false;
    unsigned int rectLeft = 0;
    for (unsigned int y = 0; y < image.getSize().y; y++)
    {
        for (unsigned int x = 0; x < image.getSize().x; x++)
        {
            const bool isTransparentPixel = (pixelData[y * image.getSize().x * 4 + x * 4 + 3] == 0);
            if (isTransparentPixel && !transparentPixelFound)
            {
                transparentPixelFound = true;
                rectLeft = x;
            }
            else if (!isTransparentPixel && transparentPixelFound)
            {
                XFillRectangle(display, pixmap, gc, rectLeft, y, x - rectLeft, 1);
                transparentPixelFound = false;
            }
        }

        if (transparentPixelFound)
        {
            XFillRectangle(display, pixmap, gc, rectLeft, y, image.getSize().x - rectLeft, 1);
            transparentPixelFound = false;
        }
    }

    // Use the black and white pixmap to define the shape of the window. All pixels that are
    // white will be kept, while all black pixels will be clipped from the window.
    XShapeCombineMask(display, wnd, ShapeBounding, 0, 0, pixmap, ShapeSet);

    // Free resources
    XFreeGC(display, gc);
    XFreePixmap(display, pixmap);
    XFlush(display);
    XCloseDisplay(display);
    return true;
}

bool setTransparency(Window wnd, unsigned char alpha)
{
    Display* display = XOpenDisplay(NULL);
    unsigned long opacity = (0xffffffff / 0xff) * alpha;
    Atom property = XInternAtom(display, "_NET_WM_WINDOW_OPACITY", false);
    if (property != None)
    {
        XChangeProperty(display, wnd, property, XA_CARDINAL, 32, PropModeReplace, (unsigned char*)&opacity, 1);
        XFlush(display);
        XCloseDisplay(display);
        return true;
    }
    else
    {
        XCloseDisplay(display);
        return false;
    }
}

sf::RenderWindow window;

int main(int argc, char ** argv) {
    window.create(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Bongo Cat for osu!", sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(MAX_FRAMERATE);
    window.setPosition(sf::Vector2i(sf::VideoMode::getDesktopMode().width - WINDOW_WIDTH, sf::VideoMode::getDesktopMode().height - WINDOW_HEIGHT));

    // loading configs
    while (!data::init()) {
        continue;
    }
    
    // initialize input
    if (!input::init()) {
        return EXIT_FAILURE;
    }

    Json::Value transparent = data::cfg["decoration"]["transparent"];
    if (transparent.asBool())
    {
        sf::Image backgroundImage;
        backgroundImage.loadFromFile("/home/np/Projects/better-bongo/img/bg_mask.png");
        setShape(window.getSystemHandle(), backgroundImage);
    }
    Json::Value opacity = data::cfg["decoration"]["opacity"];
    setTransparency(window.getSystemHandle(), opacity.asInt());


    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            switch (event.type) {
            case sf::Event::Closed:
                window.close();
                break;
            }
        }

        Json::Value rgb = data::cfg["decoration"]["rgb"];
        int red_value = rgb[0].asInt();
        int green_value = rgb[1].asInt();
        int blue_value = rgb[2].asInt();
        int alpha_value = rgb.size() == 3 ? 255 : rgb[3].asInt();

        window.clear(sf::Color(red_value, green_value, blue_value, alpha_value));
        
        custom::draw();

        window.display();
    }

    input::cleanup();
    return 0;
}
