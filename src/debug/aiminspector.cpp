#include "../../include/debug/aiminspector.h"

AimInspector* AimInspector::pInstance = nullptr;

AimInspector::AimInspector(QObject *parent) : QObject(parent)
{
}

AimInspector::~AimInspector()
{
}

AimInspector* AimInspector::GetInstance()
{
    if (AimInspector::pInstance == nullptr)
    {
        AimInspector::pInstance = new AimInspector();
    }

    return AimInspector::pInstance;
}

// The Update*Status() methods are called synchronously from the CPU's own
// QThread (Cpu::Run() -> ... -> IOComponent::UpdateDebugStatus()), but
// AimInspector lives on the GUI thread and its QString properties are read
// concurrently from QML property bindings on that same GUI thread. Writing
// to a QObject's QString property directly from a foreign thread while the
// GUI thread may be mid-read is a data race that showed up as a heap
// use-after-free (AddressSanitizer caught QV4::Heap::String::append reading
// memory QString::operator=() had just freed on the CPU thread). Marshal the
// actual property write over to AimInspector's own thread via a queued
// invocation instead of mutating it directly from the caller's thread.
void AimInspector::UpdateCpuStatus(const std::string &data)
{
    QString qData = QString::fromStdString(data);
    QMetaObject::invokeMethod(this, [this, qData]() { this->SetCpuStatus(qData); }, Qt::QueuedConnection);
}

void AimInspector::UpdateComponentStatus(const std::string &data)
{
    QString qData = QString::fromStdString(data);
    QMetaObject::invokeMethod(this, [this, qData]() { this->SetComponentStatus(qData); }, Qt::QueuedConnection);
}

void AimInspector::UpdateLedDisplayStatus(const std::string &data)
{
    QString qData = QString::fromStdString(data);
    QMetaObject::invokeMethod(this, [this, qData]() { this->SetLedStatus(qData); }, Qt::QueuedConnection);
}

void AimInspector::UpdatePrinterStatus(const std::string &data)
{
    QString qData = QString::fromStdString(data);
    QMetaObject::invokeMethod(this, [this, qData]() { this->SetPrinterStatus(qData); }, Qt::QueuedConnection);
}

void AimInspector::UpdateKeyboardStatus(const std::string &data)
{
    QString qData = QString::fromStdString(data);
    QMetaObject::invokeMethod(this, [this, qData]() { this->SetKeyboardStatus(qData); }, Qt::QueuedConnection);
}

QString AimInspector::GetComponentStatus() const
{
    return this->m_componentStatus;
}

QString AimInspector::GetCpuStatus() const
{
    return this->m_cpuStatus;
}

QString AimInspector::GetKeyboardStatus() const
{
    return this->m_keyboardStatus;
}

QString AimInspector::GetLedStatus() const
{
    return this->m_ledStatus;
}

QString AimInspector::GetPrinterStatus() const
{
    return this->m_printerStatus;
}

void AimInspector::SetCpuStatus(QString cpuStatus)
{
    this->m_cpuStatus = cpuStatus;
    emit cpuStatusChanged();
}

void AimInspector::SetLedStatus(QString ledStatus)
{
    this->m_ledStatus = ledStatus;
    emit ledStatusChanged();
}

void AimInspector::SetPrinterStatus(QString printerStatus)
{
    this->m_printerStatus = printerStatus;
    emit printerStatusChanged();
}

void AimInspector::SetKeyboardStatus(QString keyboardStatus)
{
    this->m_keyboardStatus = keyboardStatus;
    emit keyboardStatusChanged();
}

void AimInspector::SetComponentStatus(QString ledStatus)
{
    this->m_componentStatus = ledStatus;
    emit componentStatusChanged();
}

const std::string AimInspector::CurrentDateTime()
{
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

    return buf;
}

