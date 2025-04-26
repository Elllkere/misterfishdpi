#pragma once
class Singbox : public Zapret
{
public:
    int status = -1;
    std::string key_rule;
    json domains;

    Singbox(int width, int height, const std::string& name, const std::string& id_name, ID3D11ShaderResourceView* texture, const std::string& key_rule, json domains) : Zapret(width, height, name, id_name, texture, "")
    {
        this->key_rule = key_rule;
        this->domains = domains;
    }

    void writeRule();
    void restart() override;
    bool isRunning() override;
    void terminate() override;
    void start(const std::string& id_name = "") override;
private:
    bool isRunningInternal();
    void startInternal();
    bool isPrcsRunning();
};