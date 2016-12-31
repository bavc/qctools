#include "config.h"

Config &Config::instance()
{
    static Config self;
    return self;
}

Config::Config() : debug(false)
{

}

bool Config::getDebug() const
{
    return debug;
}

void Config::setDebug(bool value)
{
    debug = value;
}
