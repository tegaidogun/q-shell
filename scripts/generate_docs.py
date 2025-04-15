#!/usr/bin/env python3

import os
import re
from pathlib import Path
from collections import defaultdict

def extract_docs_from_file(file_path):
    """Extract documentation from a C source file."""
    with open(file_path, 'r') as f:
        content = f.read()
    
    docs = {
        'file': None,
        'variables': [],
        'functions': [],
        'types': []
    }
    
    # Extract file-level documentation
    file_doc_pattern = r'/\*\*\s*\n\s*\*\s*@file\s*(.*?)\*/\s*'
    file_match = re.search(file_doc_pattern, content, re.DOTALL)
    if file_match:
        docs['file'] = file_match.group(1).strip()
    
    # Extract variable documentation
    var_pattern = r'/\*\*\s*\n(.*?)\*/\s*\n\s*(?:extern\s+)?(?:static\s+)?(?:const\s+)?(?:unsigned\s+)?(?:signed\s+)?(?:long\s+)?(?:short\s+)?(?:char\s+)?(?:int\s+)?(?:void\s+)?(?:struct\s+)?(?:enum\s+)?(?:union\s+)?([a-zA-Z_][a-zA-Z0-9_]*)\s*;'
    var_matches = re.finditer(var_pattern, content, re.DOTALL)
    
    for match in var_matches:
        comment = match.group(1)
        var_name = match.group(2)
        
        # Clean up the comment
        comment = re.sub(r'^\s*\*\s*', '', comment, flags=re.MULTILINE)
        comment = comment.strip()
        
        docs['variables'].append((var_name, comment))
    
    # Extract function documentation
    func_pattern = r'/\*\*\s*\n(.*?)\*/\s*\n\s*(?:static\s+)?(?:inline\s+)?(?:const\s+)?(?:unsigned\s+)?(?:signed\s+)?(?:long\s+)?(?:short\s+)?(?:char\s+)?(?:int\s+)?(?:void\s+)?(?:struct\s+)?(?:enum\s+)?(?:union\s+)?([a-zA-Z_][a-zA-Z0-9_]*)\s*\([^)]*\)'
    func_matches = re.finditer(func_pattern, content, re.DOTALL)
    
    for match in func_matches:
        comment = match.group(1)
        func_name = match.group(2)
        
        # Clean up the comment
        comment = re.sub(r'^\s*\*\s*', '', comment, flags=re.MULTILINE)
        comment = comment.strip()
        
        # Extract parameters and return value
        params = []
        returns = None
        for line in comment.split('\n'):
            line = line.strip()
            if line.startswith('@param'):
                param_match = re.match(r'@param\s+(\S+)\s+(.*)', line)
                if param_match:
                    params.append((param_match.group(1), param_match.group(2)))
            elif line.startswith('@return'):
                returns = line[8:].strip()
        
        docs['functions'].append((func_name, comment, params, returns))
    
    # Extract struct/enum documentation
    struct_pattern = r'/\*\*\s*\n(.*?)\*/\s*\n\s*typedef\s+(?:struct|enum)\s+(?:[a-zA-Z_][a-zA-Z0-9_]*)?\s*{([^}]*)}\s*([a-zA-Z_][a-zA-Z0-9_]*)\s*;'
    struct_matches = re.finditer(struct_pattern, content, re.DOTALL)
    
    for match in struct_matches:
        comment = match.group(1)
        members = match.group(2)
        type_name = match.group(3)
        
        # Clean up the comment
        comment = re.sub(r'^\s*\*\s*', '', comment, flags=re.MULTILINE)
        comment = comment.strip()
        
        # Extract member documentation
        member_docs = []
        for line in members.split('\n'):
            line = line.strip()
            if line:
                member_match = re.match(r'([^/]*)(?:/\*\*\s*(.*?)\s*\*/)?', line)
                if member_match:
                    member = member_match.group(1).strip()
                    member_doc = member_match.group(2) if member_match.group(2) else ''
                    if member:
                        member_docs.append((member, member_doc))
        
        docs['types'].append((type_name, comment, member_docs))
    
    return docs

def generate_markdown(docs, file_path):
    """Generate markdown documentation from extracted docs."""
    if not docs:
        return ""
    
    # Get the relative path for the title
    rel_path = str(file_path).replace('src/', '').replace('include/', '')
    
    markdown = f"# {rel_path}\n\n"
    
    # Add file documentation if present
    if docs['file']:
        markdown += f"{docs['file']}\n\n"
    
    # Add variables section
    if docs['variables']:
        markdown += "## Variables\n\n"
        for var_name, comment in docs['variables']:
            markdown += f"### {var_name}\n\n"
            if comment:
                markdown += f"{comment}\n\n"
    
    # Add types section
    if docs['types']:
        markdown += "## Types\n\n"
        for type_name, comment, members in docs['types']:
            markdown += f"### {type_name}\n\n"
            if comment:
                markdown += f"{comment}\n\n"
            if members:
                markdown += "#### Members\n\n"
                for member, member_doc in members:
                    if member_doc:
                        markdown += f"- `{member}` - {member_doc}\n"
                    else:
                        markdown += f"- `{member}`\n"
                markdown += "\n"
    
    # Add functions section
    if docs['functions']:
        markdown += "## Functions\n\n"
        for func_name, comment, params, returns in docs['functions']:
            markdown += f"### {func_name}\n\n"
            
            # Split into description and parameters/returns
            desc_lines = []
            for line in comment.split('\n'):
                if not line.startswith('@'):
                    desc_lines.append(line)
            
            desc_text = ' '.join(line.strip() for line in desc_lines if line.strip())
            if desc_text:
                markdown += f"{desc_text}\n\n"
            
            if params:
                markdown += "#### Parameters\n\n"
                for param_name, param_desc in params:
                    markdown += f"- `{param_name}`: {param_desc}\n"
                markdown += "\n"
            
            if returns:
                markdown += f"#### Returns\n\n{returns}\n\n"
    
    return markdown

def generate_docs_index(project_root):
    """Generate index of all documentation files."""
    docs_dir = project_root / 'docs'
    index = "# Documentation Index\n\n"
    
    # Add main documentation files
    index += "## Main Documentation\n\n"
    index += "- [User Guide](../user_guide.md) - User guide for q-shell\n"
    index += "- [Development Guidelines](../development.md) - Guidelines for developers\n"
    index += "- [Main Documentation](main.md) - Core shell implementation details\n\n"
    
    # Add module documentation
    index += "## Module Documentation\n\n"
    
    # Core module
    index += "### Core\n\n"
    core_dir = docs_dir / 'core'
    if core_dir.exists():
        for file in sorted(core_dir.glob('*.md')):
            rel_path = file.relative_to(docs_dir)
            index += f"- [{file.stem}]({rel_path}) - Core shell functionality\n"
    
    # Builtins module
    index += "\n### Builtins\n\n"
    builtins_dir = docs_dir / 'builtins'
    if builtins_dir.exists():
        for file in sorted(builtins_dir.glob('*.md')):
            rel_path = file.relative_to(docs_dir)
            index += f"- [{file.stem}]({rel_path}) - Built-in commands\n"
    
    # Utils module
    index += "\n### Utilities\n\n"
    utils_dir = docs_dir / 'utils'
    if utils_dir.exists():
        for file in sorted(utils_dir.glob('*.md')):
            rel_path = file.relative_to(docs_dir)
            index += f"- [{file.stem}]({rel_path}) - Utility functions\n"
    
    # Profiler module
    index += "\n### Profiler\n\n"
    profiler_dir = docs_dir / 'profiler'
    if profiler_dir.exists():
        for file in sorted(profiler_dir.glob('*.md')):
            rel_path = file.relative_to(docs_dir)
            index += f"- [{file.stem}]({rel_path}) - Profiling functionality\n"
    
    return index

def process_directory(src_dir, docs_dir):
    """Process all C files in a directory and generate documentation."""
    # Track seen items to avoid duplicates
    seen_items = defaultdict(set)
    
    for root, _, files in os.walk(src_dir):
        for file in files:
            if file.endswith(('.c', '.h')):
                src_path = Path(root) / file
                rel_path = src_path.relative_to(src_dir)
                
                # Create corresponding docs directory
                docs_path = docs_dir / rel_path.parent
                docs_path.mkdir(parents=True, exist_ok=True)
                
                # Generate documentation
                docs = extract_docs_from_file(src_path)
                
                # Filter out duplicates
                filtered_docs = {
                    'file': docs['file'],
                    'variables': [],
                    'functions': [],
                    'types': []
                }
                
                # Filter variables
                for var_name, comment in docs['variables']:
                    if var_name not in seen_items['variables']:
                        seen_items['variables'].add(var_name)
                        filtered_docs['variables'].append((var_name, comment))
                
                # Filter functions
                for func_name, comment, params, returns in docs['functions']:
                    if func_name not in seen_items['functions']:
                        seen_items['functions'].add(func_name)
                        filtered_docs['functions'].append((func_name, comment, params, returns))
                
                # Filter types
                for type_name, comment, members in docs['types']:
                    if type_name not in seen_items['types']:
                        seen_items['types'].add(type_name)
                        filtered_docs['types'].append((type_name, comment, members))
                
                if any(filtered_docs.values()):
                    markdown = generate_markdown(filtered_docs, rel_path)
                    if markdown:
                        with open(docs_path / f"{rel_path.stem}.md", 'w') as f:
                            f.write(markdown)

def main():
    # Get the project root directory
    project_root = Path(__file__).parent.parent
    
    # Process source directories
    process_directory(project_root / 'src', project_root / 'docs')
    process_directory(project_root / 'include', project_root / 'docs')
    
    # Generate documentation index
    index = generate_docs_index(project_root)
    with open(project_root / 'docs' / 'index.md', 'w') as f:
        f.write(index)
    
    # Copy main documentation files to root
    main_docs = ['development.md', 'user_guide.md']
    for doc in main_docs:
        src = project_root / 'docs' / doc
        dst = project_root / doc
        if src.exists():
            with open(src, 'r') as f:
                content = f.read()
            
            # Add documentation index link
            if doc == 'development.md':
                content += "\n\n## Documentation Index\n\n"
                content += "For detailed API documentation, see the [Documentation Index](docs/index.md).\n"
            elif doc == 'user_guide.md':
                content += "\n\n## Additional Resources\n\n"
                content += "- [Development Guidelines](../development.md) - For developers\n"
                content += "- [API Documentation](docs/index.md) - Detailed API reference\n"
            
            with open(dst, 'w') as f:
                f.write(content)

if __name__ == '__main__':
    main() 