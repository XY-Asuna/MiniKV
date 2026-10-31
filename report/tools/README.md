# docx 生成

代码是agent写的。
将 [report.md](../report.md) 转换为 [report.docx](../report.docx) 。

## 依赖

- pandoc
- Python + python-docx

## 使用

```powershell
python report/tools/build_docx.py
```
```powershell
# 生成默认模板
python report/tools/build_template.py
```
## 手动修改模板

`ref-template.docx` 可以直接用 Word 打开修改样式，保存后重跑 `python report/tools/build_docx.py` 即可生效。不要运行 `build_template.py`

`build_template.py` 会从 pandoc 默认模板重新生成模板

