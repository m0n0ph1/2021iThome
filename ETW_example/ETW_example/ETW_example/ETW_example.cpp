#include <krabs.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <vector>

// ws2s: �N���O wstring �� string
std::string ws2s(const std::wstring& wstr) {
    const std::string str(wstr.begin(), wstr.end());
    return str;
}

// byte2uint32: �N���O vector<Byte> �ର uint32
uint32_t byte2uint32(std::vector<BYTE> v) {
    uint32_t res = 0;
    for (int i = (int)v.size() - 1; i >= 0; i--) {
        res <<= 8;
        res += (uint32_t)v[i];
    }
    return res;
}

// byte2uint64: �N���O vector<BYTE> �ର uint64
uint64_t byte2uint64(std::vector<BYTE> v) {
    uint64_t res = 0;
    for (int i = 7; i >= 0; i--) {
        res <<= 8;
        res += (uint64_t)v[i];
    }
    return res;
}

// callback: �� Session �q Provider �������ƥ�ɷ|�I�s
void callback(const EVENT_RECORD& record, const krabs::trace_context& trace_context) {
    // ���o�ƥ�ҫ��� Parse
    krabs::schema schema(record, trace_context.schema_locator);
    krabs::parser parser(schema);

    // �p�G�O ProcessStart �ƥ�~�B�z
    if (schema.event_id() == 1) {
        // �N�ƥ󤤪��C����� Parse ���ন�ݭn�����O�æs��� json ��
        nlohmann::json data;
        data["ProcessID"] = byte2uint32(parser.parse<krabs::binary>(L"ProcessID").bytes());
        data["CreateTime"] = byte2uint64(parser.parse<krabs::binary>(L"CreateTime").bytes());
        data["ParentProcessID"] = byte2uint32(parser.parse<krabs::binary>(L"ParentProcessID").bytes());
        data["SessionID"] = byte2uint32(parser.parse<krabs::binary>(L"SessionID").bytes());
        data["ImageName"] = ws2s(parser.parse<std::wstring>(L"ImageName"));

        // �N json ��ƦL�X�A�Ѽ� 4 ���L�X�Ӫ���Ƥ���e����
        std::cout << data.dump(4) << std::endl;
    }
}

int main(int argc, const char* argv[]) {
    // 1. �إ� Session
    krabs::user_trace session(L"ETW_example");

    // 2. �]�w Provider ��T
    krabs::provider<> provider(L"Microsoft-Windows-Kernel-Process");
    provider.any(0x10); // �o�O�W�@�g���ѹL�� flags

    // 3. �]�m Callback �B�z�ƥ�
    provider.add_on_event_callback(callback);

    // 4. �]�w Session �q�ؼ� Provider ����ƥ�
    session.enable(provider);

    // 5. �ҥ� Session
    session.start();
}