#pragma once

namespace ImGui
{
	inline bool InputText(std::string_view aName, std::string& aValue)
	{
		return ImGui::InputText(aName.data(), aValue.data(), aValue.capacity() + 1, 
			ImGuiInputTextFlags_CallbackResize, [](ImGuiInputTextCallbackData* aData)
		{
			std::string* valueStr = static_cast<std::string*>(aData->UserData);
			if (aData->EventFlag == ImGuiInputTextFlags_CallbackResize)
			{
				valueStr->resize(aData->BufTextLen);
				aData->Buf = valueStr->data();
			}
			return 0;
		}, &aValue);
	}
}