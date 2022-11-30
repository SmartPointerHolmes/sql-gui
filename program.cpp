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
    , ActiveDatabase(nullptr)
{
}

void Program::Init()
{
    if (OpenFile)
    {
        auto FilePath = OpenFile();
        int rc;
        rc = sqlite3_open(FilePath.c_str(), &ActiveDatabase);

        if (rc) {
            fprintf(stderr, "Failed to open database %s: %s", FilePath.c_str(), sqlite3_errmsg(ActiveDatabase));
            sqlite3_close(ActiveDatabase);
            exit(1);
        }
    }

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
    if ((!mAllTablesHandle || !mAllTablesHandle->IsValid()) && ActiveDatabase)
    {
        mAllTablesHandle = TableHandle::BuildTable("select name from sqlite_master where type='table'", ActiveDatabase);
        if (!mAllTablesHandle->IsValid()) {
            fprintf(stderr, "SQL error: %s\n", mAllTablesHandle->GetErrorMessage());
            mAllTablesHandle = nullptr;
        }
    }

    if (ImGui::Begin("Database", &WindowOpen))
    {
        if (ImGui::BeginTabBar("##tabs", ImGuiTabBarFlags_None)) {

            if (ImGui::BeginTabItem("Tables")) {

                DrawTablesView();
                ImGui::EndTabItem();
            }

            if (ActiveDatabase)
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
    sqlite3_close(ActiveDatabase);
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

        mSQLTableHandle = TableHandle::BuildTable(query, ActiveDatabase);

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
            FilePath = NewFile();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Open Database")) {
        if (OpenFile)
        {
            FilePath = OpenFile();
        }
    }
    if (FilePath.size() > 0)
    {
        int rc;
        if (ActiveDatabase)
        {
            mSQLTableHandle.reset();
            mAllTablesHandle.reset();
            mCurrentTableFullContents.reset();
            mSelectedTableIndex = 0;
            sqlite3_close(ActiveDatabase);
        }
        rc = sqlite3_open(FilePath.c_str(), &ActiveDatabase);

        if (rc) {
            fprintf(stderr, "Failed to open database %s: %s", FilePath.c_str(), sqlite3_errmsg(ActiveDatabase));
            sqlite3_close(ActiveDatabase);
            exit(1);
        }
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
        mCurrentTableFullContents = TableHandle::BuildTable(q, ActiveDatabase);

        if (!mCurrentTableFullContents->IsValid()) {
            fprintf(stderr, "SQL error: %s\n", mCurrentTableFullContents->GetErrorMessage());
        }
    }
}

std::shared_ptr<TableHandle> TableHandle::BuildTable(const char* Query, sqlite3* Database)
{
    std::shared_ptr<TableHandle> Table = std::make_shared <TableHandle>();
    
    const int ReturnCode = sqlite3_get_table(
        Database,
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

    return Table;
}

TableHandle::~TableHandle()
{
    if (mResult)
    {
        sqlite3_free_table(mResult);
        mResult = nullptr;
    }
}
