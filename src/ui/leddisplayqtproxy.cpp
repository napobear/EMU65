#include "../../include/ui/leddisplayqtproxy.h"

LedDisplayProxy::LedDisplayProxy(int numDisplays, int numDisplayChars, QObject *parent)
    : QObject(parent), m_numDisplays(numDisplays), m_numDisplayChars(numDisplayChars)
{
    for (int i = 0; i < this->m_numDisplays; ++i)
    {
        std::vector<byte> display;
        for (int j = 0; j < this->m_numDisplayChars; ++j)
        {
            display.push_back(0);
        }

        this->m_ledDisplays.push_back(display);
        display.clear();
    }
    // RegisterProxy() cannot be called here: shared_from_this() requires an
    // owning shared_ptr to already exist, which is only true once construction
    // (via std::make_shared at the call site) has completed. Callers must
    // invoke RegisterProxy() explicitly right after construction.
}

LedDisplayProxy::~LedDisplayProxy()
{
}

void LedDisplayProxy::RegisterProxy()
{
    // Share the same control block as the caller's owning shared_ptr instead
    // of constructing an independent std::shared_ptr<LedDisplayProxy>(this).
    UiProxyCollection::GetInstance()->InsertLedDisplayProxy(this->shared_from_this());
}

QString LedDisplayProxy::GetLedDisplay()
{
    std::string holder = "";

    for(auto display : this->m_ledDisplays)
    {
        for (auto character : display)
        {
            holder += character;
        }
    }

    return QString::fromStdString(holder);
}

void LedDisplayProxy::SetLedDisplayChar(std::pair<int,int> displayDigitPair, byte data)
{
    if(data < 32 || data > 126)
    {
        this->m_ledDisplays[std::get<0>(displayDigitPair)][std::get<1>(displayDigitPair)] = '*';
    }
    else
    {
        this->m_ledDisplays[std::get<0>(displayDigitPair)][std::get<1>(displayDigitPair)] = data;
    }
}

void LedDisplayProxy::triggerDisplayDigitChanged(std::pair<int,int> displayDigitPair, byte data)
{
    if (std::get<0>(displayDigitPair) < 0 || std::get<1>(displayDigitPair) < 0)
    {
        return;
    }
    this->SetLedDisplayChar(displayDigitPair, data);
    emit displayDigitChanged();
}
