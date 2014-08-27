/*=============================================================================

  Library: CppMicroServices

  Copyright (c) German Cancer Research Center,
    Division of Medical and Biological Informatics

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

=============================================================================*/


#include "usModuleResource.h"

#include "usAtomicInt_p.h"
#include "usModuleResourceTree_p.h"

#include <string>

US_BEGIN_NAMESPACE

class ModuleResourcePrivate
{

public:

  ModuleResourcePrivate()
    : resourceTree(NULL)
    , node(-1)
    , size(0)
    , data(NULL)
    , isFile(false)
    , isCompressed(false)
    , ref(1)
  {}

  std::string fileName;
  std::string path;
  std::string filePath;

  const ModuleResourceTree* resourceTree;

  int node;
  int32_t size;
  const unsigned char* data;

  mutable std::vector<std::string> children;

  bool isFile;
  bool isCompressed;

  /**
   * Reference count for implicitly shared private implementation.
   */
  AtomicInt ref;
};

ModuleResource::ModuleResource()
  : d(new ModuleResourcePrivate)
{
}

ModuleResource::ModuleResource(const ModuleResource &resource)
  : d(resource.d)
{
  d->ref.Ref();
}

ModuleResource::ModuleResource(const std::string& _file, ModuleResourceTree* resourceTree)
  : d(new ModuleResourcePrivate)
{
  d->resourceTree = resourceTree;

  std::string file = _file;
  if (file.empty()) file = "/";
  if (file[0] != '/') file = std::string("/") + file;

  std::size_t index = file.find_last_of('/');
  if (index < file.size()-1)
  {
    d->fileName = file.substr(index+1);
  }
  std::string rawPath = file.substr(0,index+1);

  // remove duplicate /
  std::string::value_type lastChar = 0;
  for (std::size_t i = 0; i < rawPath.size(); ++i)
  {
    if (rawPath[i] == '/' && lastChar == '/')
    {
      continue;
    }
    lastChar = rawPath[i];
    d->path.push_back(lastChar);
  }

  d->filePath = d->path + d->fileName;

  d->node = d->resourceTree->FindNode(GetResourcePath());
  if (d->node != -1)
  {
    d->isFile = !d->resourceTree->IsDir(d->node);
    if (d->isFile)
    {
      d->data = d->resourceTree->GetData(d->node, &d->size);
      d->isCompressed = d->resourceTree->IsCompressed(d->node);
    }
  }
}

ModuleResource::~ModuleResource()
{
  if (!d->ref.Deref())
    delete d;
}

ModuleResource& ModuleResource::operator =(const ModuleResource& resource)
{
  ModuleResourcePrivate* curr_d = d;
  d = resource.d;
  d->ref.Ref();

  if (!curr_d->ref.Deref())
    delete curr_d;

  return *this;
}

bool ModuleResource::operator <(const ModuleResource& resource) const
{
  return this->GetResourcePath() < resource.GetResourcePath();
}

bool ModuleResource::operator ==(const ModuleResource& resource) const
{
  return d->resourceTree == resource.d->resourceTree &&
      this->GetResourcePath() == resource.GetResourcePath();
}

bool ModuleResource::operator !=(const ModuleResource &resource) const
{
  return !(*this == resource);
}

bool ModuleResource::IsValid() const
{
  return d->resourceTree && d->resourceTree->IsValid() && d->node > -1;
}

bool ModuleResource::IsCompressed() const
{
  return d->isCompressed;
}

ModuleResource::operator bool_type() const
{
  return IsValid() ? &ModuleResource::d : NULL;
}

std::string ModuleResource::GetName() const
{
  return d->fileName;
}

std::string ModuleResource::GetPath() const
{
  return d->path;
}

std::string ModuleResource::GetResourcePath() const
{
  return d->filePath;
}

std::string ModuleResource::GetBaseName() const
{
  return d->fileName.substr(0, d->fileName.find_first_of('.'));
}

std::string ModuleResource::GetCompleteBaseName() const
{
  return d->fileName.substr(0, d->fileName.find_last_of('.'));
}

std::string ModuleResource::GetSuffix() const
{
  std::size_t index = d->fileName.find_last_of('.');
  return index < d->fileName.size()-1 ? d->fileName.substr(index+1) : std::string("");
}

std::string ModuleResource::GetCompleteSuffix() const
{
  std::size_t index = d->fileName.find_first_of('.');
  return index < d->fileName.size()-1 ? d->fileName.substr(index+1) : std::string("");
}

bool ModuleResource::IsDir() const
{
  return !d->isFile;
}

bool ModuleResource::IsFile() const
{
  return d->isFile;
}

std::vector<std::string> ModuleResource::GetChildren() const
{
  if (d->isFile || !IsValid()) return d->children;

  if (d->children.empty())
  {
    d->resourceTree->GetChildren(d->node, d->children);
  }
  return d->children;
}

int ModuleResource::GetSize() const
{
  return d->size;
}

long long ModuleResource::GetLastModified() const
{
  return -1;
}

const unsigned char* ModuleResource::GetData() const
{
  if (!IsValid()) return NULL;
  return d->data;
}

std::size_t ModuleResource::Hash() const
{
  using namespace US_HASH_FUNCTION_NAMESPACE;
  return US_HASH_FUNCTION(std::string, this->GetResourcePath());
}

US_END_NAMESPACE

US_USE_NAMESPACE

std::ostream& operator<<(std::ostream& os, const ModuleResource& resource)
{
  return os << resource.GetResourcePath();
}
