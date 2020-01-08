#pragma once
#include "../core/boostdep.hpp"

#include <QString>
#include <QAbstractItemModel>

namespace mdev {

struct DisplayFileList : QAbstractItemModel {
	DisplayFileList( const std::vector<boostdep::FileInfo>* files )
		: _infos( files )
	{
	}

	QModelIndex index( int row, int column, const QModelIndex& /*parent*/ ) const
	{
		return createIndex( row, column, (void*)&( _infos->at( row ) ) );
	}

	QModelIndex parent( const QModelIndex& /*index*/ ) const override { return QModelIndex(); }

	int rowCount( const QModelIndex& /*parent*/ = QModelIndex() ) const override { return static_cast<int>(_infos->size()); }

	int columnCount( const QModelIndex& /*parent*/ = QModelIndex() ) const override { return 2; }

	bool hasChildren( const QModelIndex& /*parent*/ = QModelIndex() ) const override { return false; }

	QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const override
	{
		if( role != Qt::DisplayRole ) {
			return {};
		}

		if( index.column() == 0 ) {
			return QString::fromStdString( std::string(_infos->at( index.row() ).module_name) );
		} else {
			return QString::fromStdString( std::string(_infos->at( index.row() ).name) );
		}
	}
	std::vector<boostdep::FileInfo> const* _infos;
};

struct TreeDisplayFileList : QAbstractItemModel {
	TreeDisplayFileList( const std::vector<boostdep::FileInfo>* files )
		: _infos( files )
	{
		int        cnt    = 0;
		const auto it_end = _infos->end();
		auto       it     = _infos->begin();
		while( it != it_end ) {
			it = next_module( it, it_end );
			cnt++;
		}
		_module_count = cnt;
	}

	QModelIndex index( int row, int column, const QModelIndex& parent ) const
	{

		if( parent.isValid() ) {
			return createIndex( row, column, (void*)&_dummy_leaf );
		} else {
			const auto end_it = _infos->end();
			auto       it     = _infos->begin();

			for( int cnt = 0; cnt < row; ++cnt ) {
				it = next_module( it, end_it );
			}
			if( it != end_it ) {
				return createIndex( row, column, (void*)&( *it ) );
			} else {
				return {};
			}
		}
	}

	QModelIndex parent( const QModelIndex& index ) const override
	{
		if( !index.isValid() || !is_leaf( index ) ) {
			return QModelIndex{};
		}
		auto module_start
			= make_reverse_iterator( ( get_internal_file_info_ptr( index ) - _infos->data() ) + _infos->begin() );
		auto list_end = _infos->rend();
		auto it       = prev_module( module_start, list_end ).base();

		auto row = module_cnt( _infos->begin(), it );
		return createIndex( row, 0, (void*)&*it );
		return {};
	}

	int rowCount( const QModelIndex& parent = QModelIndex() ) const override
	{
		if( !parent.isValid() ) {
			return _module_count;
		}
		if( is_leaf( parent ) ) {
			return 0;
		}

		auto module_start = ( get_internal_file_info_ptr( parent ) - _infos->data() ) + _infos->begin();
		auto module_end   = next_module( module_start, _infos->end() );
		return module_end - module_start;
	}

	int columnCount( const QModelIndex& parent = QModelIndex() ) const override
	{
		if( is_leaf( parent ) ) {
			return 2;
		} else {
			return 1;
		}
	}

	bool hasChildren( const QModelIndex& index = QModelIndex() ) const override
	{
		return index.isValid() && !is_leaf( index );
	}

	QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const override
	{
		if( !index.isValid() ) return QVariant();
		if( role != Qt::DisplayRole ) {
			return {};
		}
		if( !is_leaf( index ) ) {
			return QString::fromStdString( static_cast<std::string>(get_internal_file_info_ptr( index )->module_name) );
		}

		auto* parent_file = get_internal_file_info_ptr( index.parent() );
		int   row         = ( parent_file - _infos->data() ) + index.row();

		if( index.column() == 0 ) {
			return QString::fromStdString( static_cast<std::string>(_infos->at( row ).module_name) );
		} else {
			return QString::fromStdString( static_cast<std::string>(_infos->at( row ).name) );
		}
	}

	using FileList_t        = std::vector<boostdep::FileInfo>;
	using FileList_it_t     = FileList_t::const_iterator;
	using FileList_rev_it_t = FileList_t::const_reverse_iterator;

	FileList_t const* _infos;
	int               _module_count;

	static FileList_it_t next_module( FileList_it_t start, FileList_it_t end )
	{
		const auto& module_name = start->module_name;
		return std::find_if(
			start, end, [&module_name]( const boostdep::FileInfo& info ) { return info.module_name != module_name; } );
	}

	static int module_cnt( FileList_it_t start, FileList_it_t end )
	{
		int  cnt = 0;
		auto it  = start;
		while( it != end ) {
			it = next_module( it, end );
			cnt++;
		}
		return cnt;
	}

	static FileList_rev_it_t prev_module( FileList_rev_it_t start, FileList_rev_it_t end )
	{
		if( start == end ) {
			return start;
		}
		const auto& module_name = start->module_name;
		return std::find_if(
			start, end, [&module_name]( const boostdep::FileInfo& info ) { return info.module_name != module_name; } );
	}

	bool is_leaf( const QModelIndex& index ) const
	{
		return index.isValid() && ( index.internalPointer() == (void*)&_dummy_leaf );
	}

	static boostdep::FileInfo const* get_internal_file_info_ptr( const QModelIndex& index )
	{
		auto ptr = index.internalPointer();
		assert( ptr );
		return static_cast<const boostdep::FileInfo*>( ptr );
	}

	void* get_void_ptr_to_module( int idx ) const
	{
		return const_cast<void*>( static_cast<void const*>( &( _infos->at( idx ) ) ) );
	}

	int leaf_row_to_file_idx() {}

	char _dummy_leaf{};
};

}