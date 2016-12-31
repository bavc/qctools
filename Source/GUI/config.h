#ifndef CONFIG_H
#define CONFIG_H


class Config
{
public:
    static Config& instance();

    bool getDebug() const;
    void setDebug(bool value);

private:
    Config(const Config& );
    Config& operator=(const Config& );
    Config();

    bool debug;
};

#endif // CONFIG_H
