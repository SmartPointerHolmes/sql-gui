#include "program.h"
#include "imgui/imgui.h"
#include <tchar.h>
#include <stdio.h>

void DisplayTable(char** result, int rows, int cols)
{
    ImGuiTableFlags flags = 0
        | ImGuiTableFlags_Borders
        | ImGuiTableFlags_RowBg
        | ImGuiTableFlags_Resizable
        // | ImGuiTableFlags_Sortable  // we would have to sort the data ourselves
        | ImGuiTableFlags_ScrollY
        ;

    if (cols > 0 && ImGui::BeginTable("Result", cols, flags)) {

        for (int col = 0; col < cols; col++) {
            ImGui::TableSetupColumn(result[col]);
        }
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();

        for (int row = 0; row < rows; row++) {
            ImGui::TableNextRow();
            for (int col = 0; col < cols; col++) {
                ImGui::TableSetColumnIndex(col);
                const char* text = result[(row + 1) * cols + col];
                if (text == NULL) {
                    ImGui::TextDisabled("<NULL>");
                }
                else {
                    ImGui::TextUnformatted(text);
                }
            }
        }

        ImGui::EndTable();
    }
}

Program::Program(OpenFileMethod InOpenFile, OpenFileMethod InNewFile)
    : OpenFile(std::move(InOpenFile))
    , NewFile(std::move(InNewFile))
{
}

void Program::Init()
{
    auto lang = TextEditor::LanguageDefinition::SQL();
    editor.SetLanguageDefinition(lang);
    editor.SetShowWhitespaces(false);
    TextEditor::Palette palette = TextEditor::GetLightPalette();
    // disable the current line highlight, by choosing transparent colors for it.
    palette[(int)TextEditor::PaletteIndex::CurrentLineFill] = 0x00000000;
    palette[(int)TextEditor::PaletteIndex::CurrentLineFillInactive] = 0x00000000;
    palette[(int)TextEditor::PaletteIndex::CurrentLineEdge] = 0x00000000;
    editor.SetPalette(palette);
    const char* query = "select * from sqlite_master";
    editor.SetText(query);
}

bool Program::MainLoopUpdate()
{
    ImGuiIO& io = ImGui::GetIO();
    bool WindowOpen = true;
  
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Once);
    ImGui::SetNextWindowSize(io.DisplaySize, ImGuiCond_Once);

    if (ImGui::Begin("Database", &WindowOpen))
    {
        if (ImGui::BeginTabBar("##tabs", ImGuiTabBarFlags_None)) {

            if (ImGui::BeginTabItem("Tables")) {

                DrawTablesView();
                ImGui::EndTabItem();
            }

            if (mActiveDatabase)
            {
                if (ImGui::BeginTabItem("SQL")) {

                    DrawSQLQueryView();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Records")) {

                    DrawRecordsView();
                    ImGui::EndTabItem();
                }
            }
            ImGui::EndTabBar();
        }
    }

    ImGui::End();

    return !WindowOpen;
}

void Program::Shutdown()
{
    
}

void Program::DrawSQLQueryView()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGuiStyle& style = ImGui::GetStyle();

    char* err_msg = NULL;
    bool do_query = false;

    if (ImGui::GetFrameCount() == 1) do_query = true;

    ImVec2 size(
        ImGui::GetContentRegionAvail().x - 100 - style.FramePadding.x,
        ImGui::GetTextLineHeight() * 5
    );
    editor.Render("SQL", size, true);
    ImVec2 bottom_corner = ImGui::GetItemRectMax();

    ImGui::SameLine();

    if (ImGui::BeginChild("Query Buttons", ImVec2(100, 50))) {
        if (ImGui::Button("Run Query")) {
            do_query = true;
        }
        ImGui::Text("%s+Enter", io.ConfigMacOSXBehaviors ? "Cmd" : "Ctrl");
    }
    ImGui::EndChild();

    auto shift = io.KeyShift;
    auto ctrl = io.ConfigMacOSXBehaviors ? io.KeySuper : io.KeyCtrl;
    auto alt = io.ConfigMacOSXBehaviors ? io.KeyCtrl : io.KeyAlt;
    if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter), false)) {
        do_query = true;
    }

    auto cpos = editor.GetCursorPosition();
    auto selection = editor.GetSelectedText();
    char info_text[1024];
    if (selection.empty()) {
        snprintf(info_text, sizeof(info_text),
            "line %d/%d, column %d | %s",
            cpos.mLine + 1,
            editor.GetTotalLines(),
            cpos.mColumn + 1,
            editor.IsOverwrite() ? "Ovr" : "Ins");
    }
    else {
        snprintf(info_text, sizeof(info_text),
            "selected %d characters | %s",
            (int)selection.length(),
            editor.IsOverwrite() ? "Ovr" : "Ins");
    }

    ImVec2 pos = ImGui::GetCursorPos();
    ImGui::SetCursorPosX(bottom_corner.x - ImGui::CalcTextSize(info_text).x);
    ImGui::TextUnformatted(info_text);
    // ImGui::SetItemAllowOverlap();
    ImGui::SetCursorPos(pos);

    if (do_query) {
        if (err_msg) {
            sqlite3_free(err_msg);
            err_msg = NULL;
        }
        char query[1024];
        snprintf(query, sizeof(query), "%s", editor.GetText().c_str());

        mSQLTableHandle = TableHandle::BuildTable(query, mActiveDatabase);

        if (!mSQLTableHandle->IsValid()) {
            fprintf(stderr, "SQL error: %s\n", mSQLTableHandle->GetErrorMessage());
        }
    }

    if (err_msg) {
        ImGui::Text("%s", err_msg);
    }

    if (mSQLTableHandle && mSQLTableHandle->IsValid()) {
        ImGui::Text("Result %d rows, %d cols", mSQLTableHandle->GetRows(), mSQLTableHandle->GetColumns());

        DisplayTable(mSQLTableHandle->GetTable(), mSQLTableHandle->GetRows(), mSQLTableHandle->GetColumns());
    }
}

void Program::DrawTablesView()
{
    std::string FilePath;
    if (ImGui::Button("New Database")) {
        if (NewFile)
        {
            FilePath = NewFile(".db\0");
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Open Database")) {
        if (OpenFile)
        {
            FilePath = OpenFile(".db\0");
        }
    }
    if (FilePath.size() > 0)
    {
        mActiveDatabase = DatabaseHandle::CreateDatabase(FilePath);
        mAllTablesHandle = TableHandle::BuildTable("select name from sqlite_master where type='table'", mActiveDatabase);

        mSQLTableHandle.reset();
        mCurrentTableFullContents.reset();
    }
    ImGui::NewLine();

    if (mAllTablesHandle)
    {
        DrawAllTablesCombo();

        if (mCurrentTableFullContents->IsValid())
        {
            ImGui::Text("%d rows, %d cols", mCurrentTableFullContents->GetRows(), mCurrentTableFullContents->GetColumns());

            DisplayTable(mCurrentTableFullContents->GetTable(), mCurrentTableFullContents->GetRows(), mCurrentTableFullContents->GetColumns());
        }
    }
}

void Program::DrawRecordsView()
{
    if (mAllTablesHandle)
    {
        DrawAllTablesCombo();

        if (mCurrentTableFullContents->IsValid())
        {
            const int rows = mCurrentTableFullContents->GetRows();
            const int cols = mCurrentTableFullContents->GetColumns();
            char** result = mCurrentTableFullContents->GetTable();
            // Pick one record
            static int record_index = 1;
            if (record_index > rows) {
                record_index = 1;
            }

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Record %d of %d", record_index, rows);
            ImGui::SameLine();
            if (ImGui::Button("Prev")) {
                record_index--;
                if (record_index < 1) record_index = rows;
            }
            ImGui::SameLine();
            if (ImGui::Button("Next")) {
                record_index++;
                if (record_index > rows) record_index = 1;
            }

            ImGui::SliderInt("Record Index", &record_index, 1, rows);

            ImGuiTableFlags flags = 0
                | ImGuiTableFlags_Borders
                | ImGuiTableFlags_RowBg
                | ImGuiTableFlags_Resizable
                | ImGuiTableFlags_ScrollY
                ;
            if (ImGui::BeginTable("Record", 2, flags))
            {
                for (int col = 0; col < cols; col++) {
                    const char* column_name = result[col];
                    const char* value = result[record_index * cols + col];

                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted(column_name);

                    ImGui::TableSetColumnIndex(1);
                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted(value);
                }
                ImGui::EndTable();
            }
        }
    }
}

void Program::DrawAllTablesCombo()
{
    auto RawTable = mAllTablesHandle->GetTable();
    // pick a table
    int SelectedTableIndex = mSelectedTableIndex;
    ImGui::Combo("Table", &SelectedTableIndex, &RawTable[1], mAllTablesHandle->GetRows());

    if (SelectedTableIndex != mSelectedTableIndex || (!mCurrentTableFullContents || !mCurrentTableFullContents->IsValid()))
    {
        mSelectedTableIndex = SelectedTableIndex;
        static char filter[1024];
        ImGui::InputText("Filter", filter, sizeof(filter));

        char q[256];
        const char* where = filter;
        if (!strlen(where)) {
            where = "1=1";
        }
        snprintf(q, sizeof(q), "select * from %s where %s", RawTable[mSelectedTableIndex + 1], where);
        mCurrentTableFullContents = TableHandle::BuildTable(q, mActiveDatabase);

        if (!mCurrentTableFullContents->IsValid()) {
            fprintf(stderr, "SQL error: %s\n", mCurrentTableFullContents->GetErrorMessage());
        }
    }
}

std::shared_ptr<TableHandle> TableHandle::BuildTable(const char* Query, const std::shared_ptr<DatabaseHandle>& Database)
{
    std::shared_ptr<TableHandle> Table;
    if (Database)
    {
        Table = std::make_shared <TableHandle>(Database);

        const int ReturnCode = sqlite3_get_table(
            &Database->GetImpl(),
            Query,
            &Table->mResult,
            &Table->mRows,
            &Table->mColumns,
            &Table->mErrorMessage
        );

        if (ReturnCode != SQLITE_OK) {
            if (Table->mResult)
            {
                sqlite3_free_table(Table->mResult);
                Table->mResult = nullptr;
            }
        }
    }
    return Table;
}

TableHandle::TableHandle(std::shared_ptr<DatabaseHandle> Database)
{
}

TableHandle::~TableHandle()
{
    if (mResult)
    {
        sqlite3_free_table(mResult);
        mResult = nullptr;
    }
}

std::shared_ptr<DatabaseHandle> DatabaseHandle::CreateDatabase(const std::string& FilePath)
{
    std::shared_ptr<DatabaseHandle> Handle;
    sqlite3* NewDatabase = nullptr;
    const int ReturnCode = sqlite3_open(FilePath.c_str(), &NewDatabase);

    if (ReturnCode) {
        fprintf(stderr, "Failed to open database %s: %s", FilePath.c_str(), sqlite3_errmsg(NewDatabase));
        if (NewDatabase)
        {
            sqlite3_close(NewDatabase);
        }
    }
    else
    {
        Handle = std::make_shared<DatabaseHandle>(*NewDatabase);
    }
    return Handle;
}

DatabaseHandle::DatabaseHandle(sqlite3& Database)
    : mDatabase(Database)
{
}

DatabaseHandle::~DatabaseHandle()
{
    sqlite3_close(&mDatabase);
}

std::shared_ptr<TableHandle> DatabaseHandle::BuildTable(const char* Query)
{
    return TableHandle::BuildTable(Query, shared_from_this());
}
