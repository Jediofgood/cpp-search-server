#pragma once

#include<iostream>

#include"document.h"

std::ostream& operator<<(std::ostream& os, const Document& doc);

void PrintDocument(const Document& document);