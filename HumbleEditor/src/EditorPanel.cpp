#include "EditorPanel.h"

#include "Systems/EditorPanelSystem.h"

namespace HBL2::Editor
{
    void EditorPanel::SetOwner(EditorPanelSystem* owner)
    {
        m_Owner = owner;
    }

    bool EditorPanel::GotEnabled()
    {
        if (!m_PreviousEnabledState && Enabled)
        {
            m_PreviousEnabledState = Enabled;
            return true;
        }

        return false;
    }

    bool EditorPanel::GotDisabled()
    {
        if (m_PreviousEnabledState && !Enabled)
        {
            m_PreviousEnabledState = Enabled;
            return true;
        }

        if (!m_CloseState)
        {
            Enabled = false;
            m_PreviousEnabledState = false;

            m_CloseState = true;

            return true;
        }

        return false;
    }
}
