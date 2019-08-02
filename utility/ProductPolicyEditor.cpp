
#include <precomp.h>
#include <slpublic.h>
#include "utility.h"

typedef struct _ProductPolicyHeader {
    DWORD cbSize;
    DWORD cbDataSize;
    DWORD cbEndMarker;
    DWORD Unknown1;
    DWORD Unknown2;
} ProductPolicyHeader;

typedef struct _ProductPolicyValue {
    WORD cbSize;
    WORD cbName;
    WORD eDataType;
    WORD cbData;
    DWORD Unknown1;
    DWORD Unknown2;
} ProductPolicyValue;

class CProductPolicyEditor {
public:
    static CProductPolicyEditor *Init();
    CProductPolicyEditor();
    ~CProductPolicyEditor();

    void Load(const char *key, const char *value);

    ProductPolicyValue *Find(const wchar_t * name, int flag = 0);
    void Get(const wchar_t *name);
    void Set(const wchar_t *name, DWORD val);
    void Add(const wchar_t *name, DWORD val);
    void Save();
protected:
    char *_data;
    DWORD _len;
    const char *_reg_key;
    const char *_reg_value;
    ProductPolicyValue *_policy;
};

CProductPolicyEditor *ProductPolicyEditor = NULL;

CProductPolicyEditor *CProductPolicyEditor::Init()
{
    if (ProductPolicyEditor) return ProductPolicyEditor;
    ProductPolicyEditor = new CProductPolicyEditor();
    ProductPolicyEditor->Load(NULL, NULL);
    return ProductPolicyEditor;
}

void ProductPolicyLoad(const char *key, const char *value)
{
    if (!ProductPolicyEditor) CProductPolicyEditor::Init();
    if (!ProductPolicyEditor) return;
    if (key || value) {
        ProductPolicyEditor->Load(key, value);
    }
}

void ProductPolicyGet(const wchar_t *name)
{
    if (!ProductPolicyEditor) CProductPolicyEditor::Init();
    if (!ProductPolicyEditor) return;
    ProductPolicyEditor->Get(name);
}

void ProductPolicySet(const wchar_t *name, DWORD val)
{
    if (!ProductPolicyEditor) CProductPolicyEditor::Init();
    if (!ProductPolicyEditor) return;
    ProductPolicyEditor->Set(name, val);
}

void ProductPolicySave()
{
    if (!ProductPolicyEditor) CProductPolicyEditor::Init();
    if (!ProductPolicyEditor) return;
    ProductPolicyEditor->Save();
}

CProductPolicyEditor::CProductPolicyEditor()
{
    _data = NULL;
    _len = 0;
}

CProductPolicyEditor::~CProductPolicyEditor()
{
    if (_data) free(_data);
}

void CProductPolicyEditor::Load(const char *key, const char *value)
{
}

ProductPolicyValue *CProductPolicyEditor::Find(const wchar_t *key, int flag)
{
    return NULL;
}

void CProductPolicyEditor::Get(const wchar_t *name)
{
}

void CProductPolicyEditor::Set(const wchar_t *name, DWORD val)
{
}

void CProductPolicyEditor::Add(const wchar_t *name, DWORD val)
{
}

void CProductPolicyEditor::Save()
{
}

